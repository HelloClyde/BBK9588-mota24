#include <assert.h>
#include <stdio.h>
#define bda_main device_main
#include "mota24.c"
static unsigned char first[sizeof(screen)];
int main(void){
    int i,j,x,y; const int fixed[]={0,1,3,6,7,8,21,22,23,27,28,31,32,81,82,83,85,86,87,88};
    for(i=0;i<(int)(sizeof(fixed)/sizeof(fixed[0]));i++)for(j=0;j<400;j++)
        assert(sprite_pixels[sprite_index[fixed[i]]*2][j]==sprite_pixels[sprite_index[fixed[i]]*2+1][j]);
    new_game();g.floor=3;g.x=5;g.y=1;g.hp=427;g.atk=27;g.def=13;g.gold=11;
    animation_frame=0;render();for(i=0;i<(int)sizeof(screen);i++)first[i]=screen[i];
    animation_frame=1;render();
    for(y=0;y<320;y++)for(x=0;x<240;x++){
        int t=0,offset=24+(y*240+x)*2;
        if(x>=10&&x<230&&y>=56&&y<276)t=tile(g.floor,(x-10)/20,(y-56)/20);
        if(t>=121&&t<=132||t>=201||t==4||t==5)continue;
        assert(first[offset]==screen[offset]&&first[offset+1]==screen[offset+1]);
    }
    puts("PASS: walls, doors, stairs and items stable across animation frames; all nonanimated screen pixels unchanged");
    return 0;
}
