(translation_unit 
	(preproc_include (comment) @comment (#match? @comment "// *pub")) 
     @include
)
(translation_unit 
	(comment) @comment (#match? @comment "// *pub")
	(preproc_include) 
     @include
)
(translation_unit 
	(comment) @comment (#match? @comment "// *pub")
	(preproc_def) 
    @include
)
(translation_unit 
	(preproc_def (comment) @comment (#match? @comment "// *pub")) 
    @include
)
