# Build
../../../cil/bin/cilly \
  --merge --keepmerged --save-temps --doCrestInstrument -std=gnu99 \
  -I../ -I. -g -O0 -Wall -Wmissing-prototypes -Wstrict-prototypes -fexceptions \
  -DHAVE_EXPAT_CONFIG_H \
	-I../../../include \
	-L../../../lib \
  -lcrest -lstdc++ xmlparse.c xmltok.c xmlrole.c elements.c

# 위 명령어 실행뒤 생성된 a.out_comb.c 를 대상으로 crestc 를 돌린다
../../../bin/crestc a.out_comb.c
