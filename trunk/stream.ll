; llvm-as stream.ll && llvm-ld -native stream.bc lib.o -o stream && ./stream

define i32 @main() nounwind {
    tail call void @print_int(i32 100) nounwind
    call void @range(i32 0, i32 10)
    tail call void @print_int(i32 100) nounwind
    ret i32 0
}

define void @for-body(i32 %i) alwaysinline {
    tail call void @print_int(i32 %i)
    ret void
}

define void @range(i32 %a1, i32 %b) alwaysinline {
entry:
    br label %header
header:
    %a2 = phi i32 [%a1, %entry], [%a3, %while]
    %cmp = icmp slt i32 %a2, %b
    br i1 %cmp, label %while, label %while-out

while:
    call void @for-body(i32 %a2)
    %a3 = add i32 %a2, 1
    br label %header

while-out:
    ret void
}

declare void @print_int(i32)
declare i8* @llvm.frameaddress(i32)
