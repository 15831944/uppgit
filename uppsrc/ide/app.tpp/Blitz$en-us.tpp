topic "What is BLITZ";
[2 $$0,0#00000000000000000000000000000000:Default]
[l288;i1120;a17;O9;~~~.1408;2 $$1,0#10431211400427159095818037425705:param]
[a83;*R6 $$2,5#31310162474203024125188417583966:caption]
[b83;*4 $$3,5#07864147445237544204411237157677:title]
[i288;O9;C2 $$4,6#40027414424643823182269349404212:item]
[b42;a42;2 $$5,5#45413000475342174754091244180557:text]
[l288;b17;a17;2 $$6,6#27521748481378242620020725143825:desc]
[l321;tC@5;1 $$7,7#20902679421464641399138805415013:code]
[b2503;2 $$8,0#65142375456100023862071332075487:separator]
[*@(0.0.255)2 $$9,0#83433469410354161042741608181528:base]
[tC2 $$10,0#37138531426314131251341829483380:class]
[l288;a17;*1 $$11,11#70004532496200323422659154056402:requirement]
[i417;b42;a42;O9;~~~.416;2 $$12,12#10566046415157235020018451313112:tparam]
[b167;C2 $$13,13#92430459443460461911108080531343:item1]
[i288;a42;O9;C2 $$14,14#77422149456609303542238260500223:item2]
[*@2$(0.128.128)2 $$15,15#34511555403152284025741354420178:NewsDate]
[l321;*C$7;2 $$16,16#03451589433145915344929335295360:result]
[l321;b83;a83;*C$7;2 $$17,17#07531550463529505371228428965313:result`-line]
[l160;*tC+117 $$18,5#88603949442205825958800053222425:package`-title]
[2 $$19,0#53580023442335529039900623488521:gap]
[tC2 $$20,20#70211524482531209251820423858195:class`-nested]
[b50;2 $$21,21#03324558446220344731010354752573:Par]
[{_}%EN-US 
[s2; About BLITZ&]
[s0; Blitz is advanced compilation technique based on [^http`:`/`/en`.wikipedia`.org`/wiki`/Single`_Compilation`_Unit^ S
CU] approach, intended to speedup debug mode rebuilds of large 
applications. In fact, BLITZ is what allows U`+`+ to keep libraries 
in sources form. You can consider BLITZ as automated form SCU.&]
[s0; &]
[s0; Blitz processes packages (not the whole program) `- each package 
can have a single [/ blitz`-block] (blitz block  is SCU).&]
[s0; &]
[s0; Only .cpp including files with inclusion guards (#ifdef NAME`_H 
#define NAME`_H .... #endif) can qualify to be part of [/ blitz`-block]. 
&]
[s0; &]
[s0; Alternatively, you can force inclusion by&]
[s0; &]
[s7; #pragma BLITZ`_APPROVE&]
[s0; &]
[s0; (also works for header) or exlusion by&]
[s0; &]
[s7; #pragma BLITZ`_PROHIBIT.&]
[s0; &]
[s0; In addition only files older than one hour qualify for [/ blitz`-block]. 
This simple heuristics excludes file that are being worked on 
`- otherwise change in file would always require rebuilding of 
the whole package, which would be potentially slower.&]
[s0; &]
[s0; The files in package that do not qualify for [/ blitz`-block ]are 
build the regular way.&]
[s0; &]
[s0; It is also possible to disable blitz for individual packages, 
either on project basis (in Package organizer) or in Output mode 
dialog (machine specific setting).&]
[s0; &]
[s0; Files in [/ blitz`-block] are scanned for any #define, these are 
undefined after the file. [/ blitz`-block] is in fact a file generated 
into output directory that include all blitz approved files and 
gets compiled instead of them (it is named `$blitz.cpp, you can 
check the output directory for details).&]
[s0; &]
[s0; BLITZ also includes one dirty trick to basically allow BLITZ 
compilation of packages that define static variables names based 
on the line number (macro `_`_LINE`_`_). Static variables are 
not visible outside the compilation unit using the normal build 
process, but when using BLITZ, more than single .cpp files are 
compiled into single unit and so there is the possibility that 
two such variables would have the same name.&]
[s0; &]
[s0; Therefore BLITZ adds BLITZ`_INDEX`_`_ macro, which contains 
a unique number (index of file) for each .cpp. That way you can 
create `"static`" identifiers e.g. like this (and also make it 
compile without BLITZ):&]
[s0; &]
[ {{10000@(255.255.192) [s7; #define MK`_`_s`_`_(x)       s`_`_s##x&]
[s7; #define MK`_`_s`_(x)        MK`_`_s`_`_(x)&]
[s7; &]
[s7; #ifdef  BLITZ`_INDEX`_`_&]
[s7; #define STATIC`_ID            MK`_`_s`_(COMBINE(BLITZ`_INDEX`_`_, 
`_`_LINE`_`_))&]
[s7; #else&]
[s7; #define STATIC`_ID            MK`_`_s`_(`_`_LINE`_`_)&]
[s0; [tC@5;1 -|#endif]]}}&]
[s0; &]
[s0; ]