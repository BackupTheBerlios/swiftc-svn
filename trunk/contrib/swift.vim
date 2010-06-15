"syn case noignore
syn sync lines=250

syn keyword swiftBoolean        true false
syn keyword swiftConditional	if else elseif
syn keyword swiftConstant	nil self
syn keyword swiftLabel		case goto label return break continue yield
syn keyword swiftOperator	and div downto in mod not of or packed with
syn keyword swiftRepeat	        for while repeat until
syn keyword swiftStatement	routine function reader writer operator create assign scope c_call vc_call stream
syn keyword swiftStruct	        class end inherits
syn keyword swiftType		ptr array simd bool int int8 int16 int32 int64 uint uint8 uint16 uint32 uint64 real real32 real64 sat8 sat16 usat8 usat16 const index
"syn keyword swiftType		string text variant

syn keyword swiftTodo contained	TODO FIXME XXX DEBUG NOTE

    " 20010723az: When wanted, highlight the trailing whitespace -- this is
    " based on c_space_errors; to enable, use "swift_space_errors".
if exists("swift_space_errors")
    if !exists("swift_no_trail_space_error")
        syn match swiftSpaceError "\s\+$"
    endif
    if !exists("swift_no_tab_space_error")
        syn match swiftSpaceError " \+\t"me=e-1
    endif
endif



" String
if !exists("swift_one_line_string")
  syn region  swiftString matchgroup=swiftString start=+'+ end=+'+ contains=swiftStringEscape
  if exists("swift_gpc")
    syn region  swiftString matchgroup=swiftString start=+"+ end=+"+ contains=swiftStringEscapeGPC
  else
    syn region  swiftStringError matchgroup=swiftStringError start=+"+ end=+"+ contains=swiftStringEscape
  endif
else
  "wrong strings
  syn region  swiftStringError matchgroup=swiftStringError start=+'+ end=+'+ end=+$+ contains=swiftStringEscape
  if exists("swift_gpc")
    syn region  swiftStringError matchgroup=swiftStringError start=+"+ end=+"+ end=+$+ contains=swiftStringEscapeGPC
  else
    syn region  swiftStringError matchgroup=swiftStringError start=+"+ end=+"+ end=+$+ contains=swiftStringEscape
  endif

  "right strings
  syn region  swiftString matchgroup=swiftString start=+'+ end=+'+ oneline contains=swiftStringEscape
  " To see the start and end of strings:
  " syn region  swiftString matchgroup=swiftStringError start=+'+ end=+'+ oneline contains=swiftStringEscape
  if exists("swift_gpc")
    syn region  swiftString matchgroup=swiftString start=+"+ end=+"+ oneline contains=swiftStringEscapeGPC
  else
    syn region  swiftStringError matchgroup=swiftStringError start=+"+ end=+"+ oneline contains=swiftStringEscape
  endif
end
syn match   swiftStringEscape		contained "''"
syn match   swiftStringEscapeGPC	contained '""'


" syn match   swiftIdentifier		"\<[a-zA-Z_][a-zA-Z0-9_]*\>"


if exists("swift_symbol_operator")
  syn match   swiftSymbolOperator      "[+\-/*=]"
  syn match   swiftSymbolOperator      "[<>]=\="
  syn match   swiftSymbolOperator      "<>"
  syn match   swiftSymbolOperator      ":="
  syn match   swiftSymbolOperator      "[()]"
  syn match   swiftSymbolOperator      "\.\."
  syn match   swiftSymbolOperator       "[\^.]"
  syn match   swiftMatrixDelimiter	"[][]"
  "if you prefer you can highlight the range
  "syn match  swiftMatrixDelimiter	"[\d\+\.\.\d\+]"
endif

syn match  swiftNumber		"-\=\<\d\+u\?s\?[bwdqx]\?\>"
syn match  swiftFloat		"-\=\<\d\+\.\d\+[dq]\?\>"
syn match  swiftFloat		"-\=\<\d\+\.\d\+[eE]-\=\d\+[dq]\?\>"
syn match  swiftHexNumber	"\$[0-9a-fA-F]\+\>"

if exists("swift_no_tabs")
  syn match swiftShowTab "\t"
endif

syn region swiftComment	start="(\*\|#"  end="\*)\|\n" contains=swiftTodo,swiftSpaceError


if !exists("swift_no_functions")
  " array functions
  syn keyword swiftFunction	pack unpack

  " memory function
  syn keyword swiftFunction	Dispose New

  " math functions
  syn keyword swiftFunction	Abs Arctan Cos Exp Ln Sin Sqr Sqrt

  " file functions
  syn keyword swiftFunction	Eof Eoln Write Writeln
  syn keyword swiftPredefined	Input Output

  if exists("swift_traditional")
    " These functions do not seem to be defined in Turbo swift
    syn keyword swiftFunction	Get Page Put 
  endif

  " ordinal functions
  syn keyword swiftFunction	Odd Pred Succ

  " transfert functions
  syn keyword swiftFunction	Chr Ord Round Trunc
endif


if !exists("swift_traditional")

  syn keyword swiftStatement	constructor destructor implementation inherited
  syn keyword swiftStatement	interface uses
  syn keyword swiftModifier	absolute assembler external far forward inline
  syn keyword swiftModifier	virtual abstract
  syn keyword swiftAcces	private public 
  syn keyword swiftStruct	object 
  syn keyword swiftOperator	shl shr xor

  syn region swiftPreProc	start="(\*\$"  end="\*)" contains=swiftTodo
  syn region swiftPreProc	start="{\$"  end="}"

  syn region  swiftAsm		matchgroup=swiftAsmKey start="\<asm\>" end="\<end\>" contains=swiftComment,swiftPreProc


  if !exists ("swift_fpc")
    syn keyword swiftPredefined	Result
  endif

  if exists("swift_fpc")
    syn region swiftComment        start="//" end="$" contains=swiftTodo,swiftSpaceError
    syn keyword swiftStatement	fail otherwise operator
    syn keyword swiftDirective	popstack
    syn keyword swiftPredefined self
    syn keyword swiftType	ShortString AnsiString WideString
  endif

  if exists("swift_delphi")
    syn region swiftComment	start="//"  end="$" contains=swiftTodo,swiftSpaceError
    syn keyword swiftType	PAnsiChar PWideChar
    syn match  swiftFloat	"-\=\<\d\+\.\d\+[dD]-\=\d\+\>"
    syn match  swiftStringEscape	contained "#[12][0-9]\=[0-9]\="
    syn keyword swiftStruct	class dispinterface end
    syn keyword swiftException	try except raise at on finally
    syn keyword swiftStatement	out
    syn keyword swiftStatement	library package 
    syn keyword swiftStatement	initialization finalization uses exports
    syn keyword swiftStatement	property out resourcestring threadvar
    syn keyword swiftModifier	contains
    syn keyword swiftModifier	overridden reintroduce abstract
    syn keyword swiftModifier	override export dynamic name message
    syn keyword swiftModifier	dispid index stored default nodefault readonly
    syn keyword swiftModifier	writeonly implements overload requires resident
    syn keyword swiftAcces	protected published automated
    syn keyword swiftDirective	register swift cvar cdecl stdcall safecall
    syn keyword swiftOperator	as is
  endif

endif

" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_swift_syn_inits")
  if version < 508
    let did_swift_syn_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

  HiLink swiftAcces		swiftStatement
  HiLink swiftBoolean		Boolean
  HiLink swiftComment		Comment
  HiLink swiftConditional	Conditional
  HiLink swiftConstant		Constant
  HiLink swiftDelimiter	Identifier
  HiLink swiftDirective	swiftStatement
  HiLink swiftException	Exception
  HiLink swiftFloat		Float
  HiLink swiftFunction		Function
  HiLink swiftLabel		Label
  HiLink swiftMatrixDelimiter	Identifier
  HiLink swiftModifier		Type
  HiLink swiftNumber		Number
  HiLink swiftOperator		Operator
  HiLink swiftPredefined	swiftStatement
  HiLink swiftPreProc		PreProc
  HiLink swiftRepeat		Repeat
  HiLink swiftSpaceError	Error
  HiLink swiftStatement	Statement
  HiLink swiftString		String
  HiLink swiftStringEscape	Special
  HiLink swiftStringEscapeGPC	Special
  HiLink swiftStringError	Error
  HiLink swiftStruct		swiftStatement
  HiLink swiftSymbolOperator	swiftOperator
  HiLink swiftTodo		Todo
  HiLink swiftType		Type
  HiLink swiftUnclassified	swiftStatement
  "  HiLink swiftAsm		Assembler
  HiLink swiftError		Error
  HiLink swiftAsmKey		swiftStatement
  HiLink swiftShowTab		Error

  delcommand HiLink
endif


let b:current_syntax = "swift"

" vim: ts=8 sw=2

