syn case ignore
syn sync lines=250

syn keyword swiftBoolean        true false
syn keyword swiftConditional	if else elseif
syn keyword swiftConstant	nil
syn keyword swiftLabel		case goto label 
syn keyword swiftOperator	and div downto in mod not of or packed with
syn keyword swiftRepeat	        for while do 
syn keyword swiftStatement	routine function reader writer operator
syn keyword swiftStatement	program begin end const var type
syn keyword swiftStruct	        class
syn keyword swiftType		array simd bool int int8 int16 int32 int64 uint uin8 uint16 uint32 uint64 real real32 real64
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

syn match  swiftNumber		"-\=\<\d\+\>"
syn match  swiftFloat		"-\=\<\d\+\.\d\+\>"
syn match  swiftFloat		"-\=\<\d\+\.\d\+[eE]-\=\d\+\>"
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
  syn keyword swiftStatement	interface unit uses
  syn keyword swiftModifier	absolute assembler external far forward inline
  syn keyword swiftModifier	interrupt near virtual 
  syn keyword swiftAcces	private public 
  syn keyword swiftStruct	object 
  syn keyword swiftOperator	shl shr xor

  syn region swiftPreProc	start="(\*\$"  end="\*)" contains=swiftTodo
  syn region swiftPreProc	start="{\$"  end="}"

  syn region  swiftAsm		matchgroup=swiftAsmKey start="\<asm\>" end="\<end\>" contains=swiftComment,swiftPreProc

  syn keyword swiftType	ShortInt LongInt Byte Word
  syn keyword swiftType	ByteBool WordBool LongBool
  syn keyword swiftType	Cardinal LongWord
  syn keyword swiftType	Single Double Extended Comp
  syn keyword swiftType	PChar


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

  if exists("swift_gpc")
    syn keyword swiftType	SmallInt
    syn keyword swiftType	AnsiChar
    syn keyword swiftType	PAnsiChar
  endif

  if exists("swift_delphi")
    syn region swiftComment	start="//"  end="$" contains=swiftTodo,swiftSpaceError
    syn keyword swiftType	SmallInt Int64
    syn keyword swiftType	Real48 Currency
    syn keyword swiftType	AnsiChar WideChar
    syn keyword swiftType	ShortString AnsiString WideString
    syn keyword swiftType	PAnsiChar PWideChar
    syn match  swiftFloat	"-\=\<\d\+\.\d\+[dD]-\=\d\+\>"
    syn match  swiftStringEscape	contained "#[12][0-9]\=[0-9]\="
    syn keyword swiftStruct	class dispinterface
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

  if exists("swift_no_functions")
    "syn keyword swiftModifier	read write
    "may confuse with Read and Write functions.  Not easy to handle.
  else
    " control flow functions
    syn keyword swiftFunction	Break Continue Exit Halt RunError

    " ordinal functions
    syn keyword swiftFunction	Dec Inc High Low

    " math functions
    syn keyword swiftFunction	Frac Int Pi

    " string functions
    syn keyword swiftFunction	Concat Copy Delete Insert Length Pos Str Val

    " memory function
    syn keyword swiftFunction	FreeMem GetMem MaxAvail MemAvail

    " pointer and address functions
    syn keyword swiftFunction	Addr Assigned CSeg DSeg Ofs Ptr Seg SPtr SSeg

    " misc functions
    syn keyword swiftFunction	Exclude FillChar Hi Include Lo Move ParamCount
    syn keyword swiftFunction	ParamStr Random Randomize SizeOf Swap TypeOf
    syn keyword swiftFunction	UpCase

    " predefined variables
    syn keyword swiftPredefined ErrorAddr ExitCode ExitProc FileMode FreeList
    syn keyword swiftPredefined FreeZero HeapEnd HeapError HeapOrg HeapPtr
    syn keyword swiftPredefined InOutRes OvrCodeList OvrDebugPtr OvrDosHandle
    syn keyword swiftPredefined OvrEmsHandle OvrHeapEnd OvrHeapOrg OvrHeapPtr
    syn keyword swiftPredefined OvrHeapSize OvrLoadList PrefixSeg RandSeed
    syn keyword swiftPredefined SaveInt00 SaveInt02 SaveInt1B SaveInt21
    syn keyword swiftPredefined SaveInt23 SaveInt24 SaveInt34 SaveInt35
    syn keyword swiftPredefined SaveInt36 SaveInt37 SaveInt38 SaveInt39
    syn keyword swiftPredefined SaveInt3A SaveInt3B SaveInt3C SaveInt3D
    syn keyword swiftPredefined SaveInt3E SaveInt3F SaveInt75 SegA000 SegB000
    syn keyword swiftPredefined SegB800 SelectorInc StackLimit Test8087

    " file functions
    syn keyword swiftFunction	Append Assign BlockRead BlockWrite ChDir Close
    syn keyword swiftFunction	Erase FilePos FileSize Flush GetDir IOResult
    syn keyword swiftFunction	MkDir Read Readln Rename Reset Rewrite RmDir
    syn keyword swiftFunction	Seek SeekEof SeekEoln SetTextBuf Truncate

    " crt unit
    syn keyword swiftFunction	AssignCrt ClrEol ClrScr Delay DelLine GotoXY
    syn keyword swiftFunction	HighVideo InsLine KeyPressed LowVideo NormVideo
    syn keyword swiftFunction	NoSound ReadKey Sound TextBackground TextColor
    syn keyword swiftFunction	TextMode WhereX WhereY Window
    syn keyword swiftPredefined CheckBreak CheckEOF CheckSnow DirectVideo
    syn keyword swiftPredefined LastMode TextAttr WindMin WindMax
    syn keyword swiftFunction BigCursor CursorOff CursorOn
    syn keyword swiftConstant Black Blue Green Cyan Red Magenta Brown
    syn keyword swiftConstant LightGray DarkGray LightBlue LightGreen
    syn keyword swiftConstant LightCyan LightRed LightMagenta Yellow White
    syn keyword swiftConstant Blink ScreenWidth ScreenHeight bw40
    syn keyword swiftConstant co40 bw80 co80 mono
    syn keyword swiftPredefined TextChar 

    " DOS unit
    syn keyword swiftFunction	AddDisk DiskFree DiskSize DosExitCode DosVersion
    syn keyword swiftFunction	EnvCount EnvStr Exec Expand FindClose FindFirst
    syn keyword swiftFunction	FindNext FSearch FSplit GetCBreak GetDate
    syn keyword swiftFunction	GetEnv GetFAttr GetFTime GetIntVec GetTime
    syn keyword swiftFunction	GetVerify Intr Keep MSDos PackTime SetCBreak
    syn keyword swiftFunction	SetDate SetFAttr SetFTime SetIntVec SetTime
    syn keyword swiftFunction	SetVerify SwapVectors UnPackTime
    syn keyword swiftConstant	FCarry FParity FAuxiliary FZero FSign FOverflow
    syn keyword swiftConstant	Hidden Sysfile VolumeId Directory Archive
    syn keyword swiftConstant	AnyFile fmClosed fmInput fmOutput fmInout
    syn keyword swiftConstant	TextRecNameLength TextRecBufSize
    syn keyword swiftType	ComStr PathStr DirStr NameStr ExtStr SearchRec
    syn keyword swiftType	FileRec TextBuf TextRec Registers DateTime
    syn keyword swiftPredefined DosError

    "Graph Unit
    syn keyword swiftFunction	Arc Bar Bar3D Circle ClearDevice ClearViewPort
    syn keyword swiftFunction	CloseGraph DetectGraph DrawPoly Ellipse
    syn keyword swiftFunction	FillEllipse FillPoly FloodFill GetArcCoords
    syn keyword swiftFunction	GetAspectRatio GetBkColor GetColor
    syn keyword swiftFunction	GetDefaultPalette GetDriverName GetFillPattern
    syn keyword swiftFunction	GetFillSettings GetGraphMode GetImage
    syn keyword swiftFunction	GetLineSettings GetMaxColor GetMaxMode GetMaxX
    syn keyword swiftFunction	GetMaxY GetModeName GetModeRange GetPalette
    syn keyword swiftFunction	GetPaletteSize GetPixel GetTextSettings
    syn keyword swiftFunction	GetViewSettings GetX GetY GraphDefaults
    syn keyword swiftFunction	GraphErrorMsg GraphResult ImageSize InitGraph
    syn keyword swiftFunction	InstallUserDriver InstallUserFont Line LineRel
    syn keyword swiftFunction	LineTo MoveRel MoveTo OutText OutTextXY
    syn keyword swiftFunction	PieSlice PutImage PutPixel Rectangle
    syn keyword swiftFunction	RegisterBGIDriver RegisterBGIFont
    syn keyword swiftFunction	RestoreCRTMode Sector SetActivePage
    syn keyword swiftFunction	SetAllPallette SetAspectRatio SetBkColor
    syn keyword swiftFunction	SetColor SetFillPattern SetFillStyle
    syn keyword swiftFunction	SetGraphBufSize SetGraphMode SetLineStyle
    syn keyword swiftFunction	SetPalette SetRGBPalette SetTextJustify
    syn keyword swiftFunction	SetTextStyle SetUserCharSize SetViewPort
    syn keyword swiftFunction	SetVisualPage SetWriteMode TextHeight TextWidth
    syn keyword swiftType	ArcCoordsType FillPatternType FillSettingsType
    syn keyword swiftType	LineSettingsType PaletteType PointType
    syn keyword swiftType	TextSettingsType ViewPortType

    " string functions
    syn keyword swiftFunction	StrAlloc StrBufSize StrCat StrComp StrCopy
    syn keyword swiftFunction	StrDispose StrECopy StrEnd StrFmt StrIComp
    syn keyword swiftFunction	StrLCat StrLComp StrLCopy StrLen StrLFmt
    syn keyword swiftFunction	StrLIComp StrLower StrMove StrNew StrPas
    syn keyword swiftFunction	StrPCopy StrPLCopy StrPos StrRScan StrScan
    syn keyword swiftFunction	StrUpper
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

