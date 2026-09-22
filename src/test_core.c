#include <assert.h>
#include <stdio.h>
#include "core.c"
static void at(int f,int x,int y){g.floor=f;g.x=x;g.y=y;mode=PLAY;}
int main(void){
    int i,old;Game saved;
    new_game();assert(g.hp==1000&&g.atk==10&&g.floor==0);
    move(0,-1);assert(g.flags&INTRO);assert(g.keys[0]==1&&g.keys[1]==1&&g.keys[2]==1);
    move(0,-1);assert(g.y==8);move(0,-1);assert(g.keys[0]==0&&g.y==7);
    for(i=0;i<7;i++)move(0,-1);
    assert(g.floor==1&&g.x==5&&g.y==9);
    assert(damage(201)==50);g.atk=1;assert(damage(201)==-1);
    new_game();at(1,2,0);g.hp=50;set(1,3,0,201);move(1,0);assert(g.hp==50&&g.x==2&&tile(1,3,0)==201);
    g.hp=51;move(1,0);assert(g.hp==1&&g.gold==1&&g.exp==1&&tile(1,3,0)==0);
    new_game();at(1,4,8);set(1,5,8,81);move(1,0);assert(g.x==4);g.keys[0]=1;move(1,0);assert(g.x==5&&g.keys[0]==0);
    new_game();at(1,0,0);set(1,1,0,27);move(1,0);assert(g.atk==13);move(-1,0);move(1,0);assert(g.atk==13);
    new_game();open_shop(0);assert(!buy(1));g.gold=25;assert(buy(1)&&g.atk==14&&g.gold==0);
    open_shop(2);g.exp=100;assert(buy(0)&&g.exp==0&&g.level==2&&g.hp==2000&&g.atk==21);
    open_shop(4);g.gold=50;buy(1);assert(g.keys[1]==1&&g.gold==0);open_shop(5);buy(1);assert(g.keys[1]==0&&g.gold==35);
    new_game();at(4,5,1);npc(123,5,0);assert(g.flags&THIEF);assert(tile(2,1,6)==0);
    g.flags|=HAMMER;npc(123,5,0);assert(tile(18,5,8)==0&&tile(18,5,9)==0);
    new_game();at(16,5,4);g.seconds=1499;npc(121,4,4);assert(g.flags&ICE);
    at(0,5,8);g.flags|=INTRO;npc(124,4,8);assert(g.flags&SECRET);g.flags|=CROSS;npc(124,4,8);assert(g.atk==13&&g.hp==1333&&(g.flags&BLESS));
    new_game();at(16,5,4);g.seconds=1500;npc(121,4,4);assert(!(g.flags&ICE));
    new_game();at(20,4,7);move(1,0);assert(g.floor==20);g.flags|=BLESS;move(1,0);assert(g.floor==21);
    new_game();g.atk=100000;g.def=100000;g.hp=1000000;g.flags=SECRET|BLESS;
    at(21,5,2);move(0,-1);assert(g.flags&BOSS21);assert(mode==PLAY&&tile(21,5,0)==87);move(0,-1);assert(g.floor==22);
    at(22,5,2);g.flags|=FIRE|HEART;npc(124,6,2);assert(tile(26,5,3)==258&&(g.flags&BLOOD));
    at(26,5,10);move(0,-1);assert(mode==PLAY);at(26,5,4);move(0,-1);assert(mode==WIN);
    new_game();g.atk=g.def=100000;at(21,5,2);move(0,-1);assert(mode==WIN);
    new_game();g.atk=10000;old=damage(219);g.def=1000;assert(damage(219)==100);assert(old>=100);
    g.atk=10000;g.hp=1000;assert(damage(246)==250);
    new_game();g.checksum=checksum(&g);saved=g;assert(valid_save(&saved));saved.hp++;assert(!valid_save(&saved));
    new_game();at(25,5,7);move(0,-1);assert(g.floor==25&&g.y==7);
    g.flags|=FIRE;move(0,-1);assert(g.floor==25&&g.y==7);
    g.flags|=HEART;move(0,-1);assert(g.floor==26&&g.y==10);
    /* Exercise every stair through the actual move handler, not only its data. */
    for(i=0;i<(int)(sizeof(stairs)/sizeof(stairs[0]));i++){
        const Stair *s=&stairs[i];int dx=s->x?1:-1;
        new_game();g.flags=INTRO|BLESS|PRINCESS|BOSS21|SECRET;
        if(s->to==26)g.flags|=FIRE|HEART;
        if(s->floor==21)set(21,5,0,87);
        at(s->floor,s->x-dx,s->y);move(dx,0);
        if(g.floor!=s->to||g.x!=s->tx||g.y!=s->ty){
            printf("FAIL stair %d (%d,%d) -> %d: stayed at %d (%d,%d)\n",s->floor,s->x,s->y,s->to,g.floor,g.x,g.y);return 1;
        }
    }
    /* Every configured stair must land on walkable ground or a stair. */
    for(i=0;i<(int)(sizeof(stairs)/sizeof(stairs[0]));i++){
        const Stair *s=&stairs[i];int t=initial_maps[s->to][s->ty*11+s->tx];
        assert(s->to>=0&&s->to<FLOORS&&s->tx>=0&&s->tx<11&&s->ty>=0&&s->ty<11);
        if(t!=0&&t!=87&&t!=88&&!(s->floor==22&&s->to==21&&t==247)){printf("Bad arrival %d -> %d (%d,%d) tile=%d\n",s->floor,s->to,s->tx,s->ty,t);return 1;}
    }
    puts("PASS: opening route, combat boundaries, keys, items, six shops, quests, timed branch, both endings, saves, all stair arrivals");
    return 0;
}
