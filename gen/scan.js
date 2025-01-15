const Parser = require('tree-sitter');
const C = require('tree-sitter-c');
const { mkdir } = require('node:fs/promises');
const { resolve, relative } = require('node:path');

console.log('________');

const parser = new Parser();
parser.setLanguage(C);


const project_root = resolve(__dirname, '..');
const buildir = process.env.BUILDIR || resolve(project_root, 'build');
const outdir = process.env.OUTDIR || resolve(project_root, 'out');
const srcdir = process.env.SRCDIR || resolve(project_root, 'src');

const dyn_c_filepath_abs = resolve(process.cwd(), process.argv[2]);
const dyn_c_path_from_src = relative(srcdir, dyn_c_filepath_abs);
const dyn_c_filepath_from_project_root = relative(project_root, dyn_c_filepath_abs);
const so_path = dyn_c_path_from_src.replace(/\.c$/, '.so');
const so_path_abs = resolve(outdir, so_path);

const gen_path = resolve(srcdir, dyn_c_path_from_src).replace(/\.c$/, '.gen.h');

console.log('Processing', dyn_c_filepath_from_project_root);
// console.log('Abs', dyn_c_filepath_abs);

const dyn_c_src = await Bun.file(dyn_c_filepath_abs).text();
const tree = parser.parse(dyn_c_src);

const query_src = await Bun.file(resolve(__dirname, '../ts_query.scm')).text();
const q = new Parser.Query(C, query_src);
const captures = q.captures(tree.rootNode);

const args_q = new Parser.Query(C, `
(function_definition 
    declarator: (function_declarator
      parameters: (parameter_list
        (parameter_declaration
            declarator: [
              (identifier) @arg_ident
              (_ declarator: (identifier) @arg_ident)
              (_ declarator: (_ declarator: (identifier)@arg_ident) )
              (_ declarator: (_ declarator: (_ declarator: (identifier)@arg_ident)) )
              (_ declarator: (_ declarator: (_ declarator: (_ declarator: (identifier)@arg_ident))) )
              (_ declarator: (_ declarator: (_ declarator: (_ declarator: (_ declarator: (identifier)@arg_ident)))) )
              (_ declarator: (_ declarator: (_ declarator: (_ declarator: (_ declarator: (_ declarator: (identifier)@arg_ident))))) )
            ]
        ) @arg
      )
    )
)
`);

await mkdir('../build', { recursive: true });

const symbols = {};

const matches = q.matches(tree.rootNode);
for (const m of matches) {
    const nodes = {};
    for (const {name, node} of m.captures) {
        nodes[name] = node;
    }
    const symbol = symbols[nodes.symbol_name.text] = {
        name: nodes.symbol_name.text,
        match: m,
        nodes,
    };
    if (m.setProperties.type === 'function') {
        const signature = nodes['signature.ret'].text + ' ' + nodes['signature.header'].text;
        const args_match = args_q.matches(nodes.func_def);
        const args = args_match.map(am => {
            const arg_fields = {};
            for (const arg_capture of am.captures) {
                const {name, node} = arg_capture;
                arg_fields[name] = node;
            }
            return {
                match: am,
                nodes: arg_fields,
                node_full: arg_fields.arg,
                node_ident: arg_fields.arg_ident,
                name: arg_fields.arg_ident.text,
            };
        });
        symbol.is_function = true;
        symbol.args = args;
        symbol.signature = signature;
    }
    console.log('Found symbol:', nodes.symbol_name.text, symbol.is_function ? '<function>': '');
}


let hosted = '';
let updater = '';

let header_case = '';

hosted += '\nvoid reg_dyn (char* mpath, char* symbol, void** target, void* registrar);\n';
for (const key of Object.keys(symbols).sort()) {
    const s = symbols[key];
    hosted += '\n';
    if (s.is_function) {
        let header_signature = s.signature;
        for (const arg of s.args) {
            header_signature = header_signature.replace(arg.name, '');
        }
        header_case += header_signature + ';\n';
        hosted += 'static ' + s.signature.replace(s.name, `(* __ptr_${s.name})`) + ' = 0;\n';
        hosted += s.signature + ' {\n';
        hosted += `  static int inited = 0; 
  if (!inited) {
    inited = 1; 
    printf("[PROXY.init] initing ${s.name}\\n");
    reg_dyn(
        "${so_path_abs}",
        "${s.name}",
        (void*)&__ptr_${s.name},
        //(void*)&inited
        "${dyn_c_filepath_from_project_root}"
    );
  }\n`;
        hosted += '  if (__ptr_' + s.name + ') {\n';
        hosted += '    return __ptr_' + s.name + '(' + s.args.map(arg => arg.name).join(', ') + ');\n';
        hosted += '  }\n';
        const ret_type = s.nodes['signature.ret'].text;
        if (ret_type !== 'void') {
          hosted+=`  ${ret_type} ___ret = {0};\n`;
          hosted+='  return ___ret;\n';
        }
        hosted += '}\n';
    }
}

const guard = dyn_c_path_from_src.replace(/[\/.-]/g, '_').toUpperCase();

hosted = `

#ifndef _DYN_SPLIT_BUILD

${header_case}

#else

extern int printf (const char *__restrict __format, ...);
${hosted}

#endif
`;



const header_inc_q = new Parser.Query(C, '(translation_unit (preproc_include (comment) @comment (#match? @comment "// *pub")) @include)');
for (const m of header_inc_q.matches(tree.rootNode)) {
    for (const c of m.captures) {
        if (c.name === 'include') {
            hosted = c.node.text.replace(/\s*\/\/.+/, '') + hosted;
        }
    }
}

hosted = `
#ifndef ${guard}
#define ${guard}

${hosted}
#endif
`;

hosted = '/**\n * File is generated\n **/\n' + hosted;

// console.log(hosted);

const genfile = Bun.file(gen_path);
if (await genfile.exists()) {
    // console.log('Previously generated file is found at', gen_path);
    const existing_txt = await genfile.text();
    if (existing_txt === hosted) {
        console.log('Gen files match. Not rewriting.');
        process.exit(0);
    }
}

await Bun.write(genfile, hosted);
console.log('File written at', gen_path);

