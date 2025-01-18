const Parser = require('tree-sitter');
const C = require('tree-sitter-c');
const { mkdir } = require('node:fs/promises');
const { resolve, relative } = require('node:path');

const parser = new Parser();
parser.setLanguage(C);

const verbose = process.argv.includes('--verbose');
const use_relative_so_path = process.argv.includes('--relative-so');

const project_root = resolve(__dirname, '../..');
const buildir = process.env.BUILDIR || resolve(project_root, 'build');
const outdir = process.env.OUTDIR || resolve(project_root, 'out');
const srcdir = process.env.SRCDIR || resolve(project_root, 'src');

const dyn_c_filepath_abs = resolve(process.cwd(), process.argv[2]);
const dyn_c_path_from_src = relative(srcdir, dyn_c_filepath_abs);
const dyn_c_filepath_from_project_root = relative(project_root, dyn_c_filepath_abs);
const so_path = dyn_c_path_from_src.replace(/\.c$/, '.so');
const so_path_abs = resolve(outdir, so_path);

const gen_path = resolve(srcdir, dyn_c_path_from_src).replace(/\.c$/, '.gen.h');

if (verbose) console.log('Processing', dyn_c_filepath_from_project_root);
// console.log('Abs', dyn_c_filepath_abs);

const dyn_c_src = await Bun.file(dyn_c_filepath_abs).text();
const tree = parser.parse(dyn_c_src);

const query_expot_symbols_src = await Bun.file(resolve(__dirname, '../ts_query_export_symbols.scm')).text();
const query_header_markers_src = await Bun.file(resolve(__dirname, '../ts_query_header_includes.scm')).text();
const q_symbols = new Parser.Query(C, query_expot_symbols_src);
const q_h_margers = new Parser.Query(C, query_expot_symbols_src);
const captures = q_symbols.captures(tree.rootNode);

const q_args = new Parser.Query(C, `
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

const matches = q_symbols.matches(tree.rootNode);
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
        const args_match = q_args.matches(nodes.func_def);
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
    if (verbose) console.log('Found symbol:', nodes.symbol_name.text, symbol.is_function ? '<function>': '');
}


let output = '';
let updater = '';

let header_case = '';

output += '\nextern void cdynsplit_reg (char* mpath, char* symbol, void** target, void* registrar);\n';
for (const key of Object.keys(symbols).sort()) {
    const s = symbols[key];
    output += '\n';
    if (s.is_function) {
        let header_signature = s.signature;
        for (const arg of s.args) {
            header_signature = header_signature.replace(arg.name, '');
        }
        header_case += header_signature + ';\n';
        output += 'static ' + s.signature.replace(s.name, `(* __ptr_${s.name})`) + ' = 0;\n';
        output += s.signature + ' {\n';
        output += `  static int inited = 0; 
  if (!inited) {
    inited = 1; 
    printf("[PROXY.init] initing ${s.name}\\n");
    cdynsplit_reg(
        "${use_relative_so_path ? so_path : so_path_abs}",
        "${s.name}",
        (void*)&__ptr_${s.name},
        "${dyn_c_filepath_from_project_root}"
    );
  }\n`;
        output += '  if (__ptr_' + s.name + ') {\n';
        output += '    return __ptr_' + s.name + '(' + s.args.map(arg => arg.name).join(', ') + ');\n';
        output += '  }\n';
        const ret_type = s.nodes['signature.ret'].text;
        if (ret_type !== 'void') {
          output+=`  ${ret_type} ___ret = {0};\n`;
          output+='  return ___ret;\n';
        }
        output += '}\n';
    }
}

const guard = dyn_c_path_from_src.replace(/[\/.-]/g, '_').toUpperCase();

output = `

#ifndef _DYN_SPLIT_BUILD

${header_case}

#else

extern int printf (const char *__restrict __format, ...);
${output}

#endif
`;



for (const m of q_h_margers.matches(tree.rootNode)) {
    for (const c of m.captures) {
        if (c.name === 'include') {
            output = c.node.text.replace(/\s*\/\/.+/, '') + output;
        }
    }
}

output = `
#ifndef ${guard}
#define ${guard}

${output}
#endif
`;

output = '/**\n * File is generated\n **/\n' + output;

// console.log(output);

const genfile = Bun.file(gen_path);
if (await genfile.exists()) {
    // console.log('Previously generated file is found at', gen_path);
    const existing_txt = await genfile.text();
    if (existing_txt === output) {
        if (verbose) console.log('Gen files match. Not rewriting.');
        process.exit(0);
    }
}

await Bun.write(genfile, output);
if (verbose) console.log('File written at', gen_path);

