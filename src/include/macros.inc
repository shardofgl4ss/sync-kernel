%ifndef AS_MACROS
%define AS_MACROS

%macro mpush 1-*
        %rep %0
                push    %1
                %rotate 1
        %endrep
%endmacro
%macro mpop 1-*
        %rep %0
                %rotate -1
                pop     %1
        %endrep
%endmacro


%macro repeat 0
        %push repeat
        %$begin:
%endmacro

%macro until 1
        %ifnctx repeat
                %error "no context to repeat to!"
        %endif
                j%-1    %$begin
        %$end:
        %pop repeat
%endmacro

%macro break 0
        %ifnctx repeat
                %error "no loop to jump to!"
        %endif
        jmp     %$end
%endmacro

%macro continue 0
        %ifnctx repeat
                %error "no loop to jump to!"
        %endif
        jmp     %$begin
%endmacro


%macro defstr 2 ; name, string (in backticks), no newline
    %1:     db %2, 0
    %1_len: equ $ - %1 - 1
%endmacro

%macro defstr_nl 2 ; name, string (in backticks), has newline
    %1: db %2, 10, 0
    %1_len: equ $ - %1 - 1
%endmacro


%endif
