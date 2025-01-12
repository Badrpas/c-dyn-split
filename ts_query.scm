(translation_unit 
    (declaration 
        (storage_class_specifier)? @spec
        type: _
        declarator: [
            (identifier) @symbol_name
            (init_declarator declarator: _ @symbol_name)
            (array_declarator declarator: _ @symbol_name)
            (pointer_declarator declarator: _ @symbol_name)
        ] @delcarator
        (#set! type "variable")
    ) @var
    (#not-eq? @spec "extern")
    (#not-eq? @spec "static")
)

(translation_unit 
    (function_definition 
        (storage_class_specifier)? @spec
        type: (_) @signature.ret
        declarator: (function_declarator
            declarator: _ @symbol_name
            parameters: (parameter_list
                (((parameter_declaration)+) @args)
            ) @params
        ) @signature.header
        (#set! type "function")
    ) @func_def
    (#not-eq? @spec "extern")
    (#not-eq? @spec "static")
)

