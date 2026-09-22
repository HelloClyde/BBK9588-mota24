/* Host UI regression. Firmware-dependent save/exit actions are tested in QEMU. */
#include <assert.h>
#include <stdio.h>
#define bda_main device_main
#include "mota24.c"
int main(void){
    int i;
    assert(direction_repeat(1,0,1)==0);
    assert(direction_repeat(1,11,1)==0);
    assert(direction_repeat(1,12,1)==1);
    assert(direction_repeat(1,16,1)==0);
    assert(direction_repeat(1,17,1)==1);
    assert(direction_repeat(0,18,1)==0);
    assert(direction_repeat(0,100,1)==0);
    assert(direction_repeat(2,101,1)==0);
    assert(direction_repeat(2,113,1)==2);
    assert(direction_repeat(4,114,1)==0);
    assert(direction_repeat(4,126,0)==0);
    assert(direction_repeat(16,150,1)==0);
    assert(direction_repeat(32,170,1)==0);
    assert(direction_repeat(3,190,1)==0);
    puts("PASS direction repeat: delay, rate, release, direction change, modal gating, no ESC/Enter repeat");
    new_game();g.x=5;g.y=5;g.map[g.floor][5*11+6]=0;g.map[g.floor][5*11+7]=0;
    input_now=200;walk(1,0);assert(g.x==6&&hero_frame==1);
    animate_hero(204);assert(hero_frame==1);
    input_now=205;walk(1,0);assert(g.x==7&&hero_frame==3);
    animate_hero(210);assert(hero_frame==0);
    g.map[g.floor][5*11+8]=1;input_now=211;walk(1,0);assert(g.x==7&&hero_frame==0);
    puts("PASS walking frames: alternate successful steps, stop idle, wall does not animate");
    const u32 esc=1u<<4;
    previous=0;touch_active=touch_escape_suppressed=escape_pending=0;
    assert(input_edges(esc,0)==0);
    assert(input_edges(0,4)==esc); /* Real short press is preserved. */
    touch_active=1;suppress_touch_escape();
    for(i=5;i<100;i++)assert(!(input_edges(esc,i)&esc));
    touch_active=0;suppress_touch_escape();
    for(i=100;i<200;i++)assert(!(input_edges(esc,i)&esc));
    assert(input_edges(esc|1,200)==1); /* Other keys are not masked. */
    assert(input_edges(0,201)==0&&!touch_escape_suppressed);
    assert(input_edges(esc,202)==0);
    assert(input_edges(esc,206)==esc);
    assert(input_edges(esc,210)==0); /* Held real key fires once. */
    input_edges(0,211);
    assert(input_edges(esc,212)==0);
    touch_active=1;suppress_touch_escape(); /* Packet precedes touch message. */
    assert(input_edges(esc,216)==0);
    touch_active=0;suppress_touch_escape();input_edges(0,217);
    assert(input_edges(esc,218)==0&&input_edges(0,222)==esc);
    puts("PASS input: touch ESC hold/release, early packet, real ESC rearm, unrelated keys");
    mode=MENU;touch((80u<<16)|80u);assert(mode==MENU);
    new_game();book();assert(mode==PLAY);fly();assert(mode==PLAY);
    g.flags|=BOOKFLAG|FLYFLAG;g.floor=1;
    book();assert(mode==BOOK&&bookpage==0);
    for(i=0;i<30;i++)action(0);
    assert(bookpage==2);render();
    for(i=0;i<30;i++)action(1);
    assert(bookpage==0);action(4);assert(mode==PLAY);
    g.visited=(1u<<0)|(1u<<1)|(1u<<9);fly();assert(mode==FLY&&selection==1);
    action(2);assert(selection==9);action(2);assert(selection==9);
    action(3);assert(selection==1);action(5);assert(mode==PLAY&&g.floor==1);
    g.floor=21;fly();assert(mode==PLAY);
    for(i=0;i<6;i++){
        int hp,atk,def;
        new_game();g.gold=g.exp=10000;g.keys[0]=g.keys[1]=g.keys[2]=3;
        hp=g.hp;atk=g.atk;def=g.def;open_shop(i);render();
        action(5);action(2);action(5);action(2);action(5);
        if(i<4)assert(g.hp>hp&&g.atk>atk&&g.def>def);
        if(i==4)assert(g.keys[0]==4&&g.keys[1]==4&&g.keys[2]==4&&g.gold==9840);
        if(i==5)assert(g.keys[0]==2&&g.keys[1]==2&&g.keys[2]==2&&g.gold==10112);
        action(4);assert(mode==PLAY);
    }
    new_game();action(5);assert(mode==MENU);for(i=0;i<6;i++)action(2);
    action(5);assert(mode==RESTART);action(4);assert(mode==PLAY);
    g.floor=8;action(5);for(i=0;i<6;i++)action(2);action(5);action(5);
    assert(g.floor==0&&mode==OPENING);action(4);assert(mode==PLAY);
    mode=TITLE;selection=0;action(5);assert(mode==OPENING&&story_page==0);
    action(5);assert(mode==OPENING&&story_page==1);for(i=1;i<PAGE_COUNT(opening_pages);i++)action(5);assert(mode==PLAY);
    mode=TITLE;selection=2;action(5);assert(mode==HELP);action(4);assert(mode==TITLE);
    selection=0;touch((188u<<16)|100u);assert(mode==OPENING);action(4);assert(mode==PLAY);
    g.floor=26;g.x=4;g.y=5;g.atk=g.def=g.hp=1000000;g.map[26][5*11+5]=257;
    walk(1,0);assert(mode==WIN&&story_page==0);action(5);assert(mode==WIN&&story_page==1);
    action(5);assert(mode==TITLE);action(4);assert(mode==TITLE);
    puts("PASS title/opening/help/skip and actual final-enemy ending transition");
    new_game();input_now=100;walk(0,-1);assert(mode==DIALOG&&dialog_count==16);
    assert(g.keys[0]==1&&g.keys[1]==1&&g.keys[2]==1);
    {int x=g.x,y=g.y;action(3);action(4);assert(mode==DIALOG&&g.x==x&&g.y==y&&dialog_page==0);}
    for(i=0;i<16;i++)action(5);assert(mode==PLAY);
    g.x=4;g.y=9;walk(0,-1);assert(dialog_pages==fairy_reminder&&g.keys[0]==1);
    for(i=0;i<dialog_count;i++)action(5);
    g.flags|=ICE;walk(0,-1);assert(dialog_pages==fairy_ice&&(g.flags&SECRET));
    for(i=0;i<dialog_count;i++)action(5);
    g.flags|=CROSS;g.hp=900;walk(0,-1);assert(dialog_pages==fairy_bless&&g.hp==1200&&(g.flags&BLESS));
    for(i=0;i<dialog_count;i++)action(5);walk(0,-1);assert(g.hp==1200&&mode==PLAY);
    g.floor=22;g.x=5;g.y=5;g.map[22][4*11+5]=124;walk(0,-1);assert(dialog_pages==fairy_hidden);
    for(i=0;i<dialog_count;i++)action(5);
    g.flags|=FIRE|HEART;walk(0,-1);assert(dialog_pages==fairy_unseal&&(g.flags&BLOOD));
    for(i=0;i<dialog_count;i++)action(5);
    puts("PASS fairy: first/revisit/ice/cross/hidden/unseal, dialogue input lock and single rewards");

    for(i=PLAY;i<=OPENING;i++){mode=i;selection=0;render();}
    puts("PASS UI: book gates/pagination, fly visited bounds, six shops, restart/cancel, all panel rendering");
    return 0;
}
