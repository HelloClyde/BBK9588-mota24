#include <assert.h>
#include <stdio.h>
#define bda_main device_main
#include "mota24.c"
int main(void){
 int id,count=0,variant;u32 now;
 for(variant=0;variant<3;variant++)for(id=201;id<300;id++)if(enemy_base[id].hp){
  Enemy e;int predicted,hp,gold,exp,steps=0;
  new_game();g.floor=1;g.x=4;g.y=5;g.flags=variant==1?BOSS16:variant==2?BOSS16|BOSS19|BOSS21:0;
  e=enemy(id);g.atk=e.def+max(1,e.hp/5);g.def=e.atk/2;g.hp=1000000;
  g.map[1][60]=id;predicted=damage(id);assert(predicted>=0&&predicted<g.hp);
  hp=g.hp;gold=g.gold;exp=g.exp;input_now=0;walk(1,0);assert(mode==BATTLE);
  action(3);action(4);assert(g.x==4&&g.hp==hp&&mode==BATTLE);
  now=0;while(battle_phase!=3){now+=6;advance_battle(now);assert(++steps<100);assert(g.hp==hp&&g.gold==gold);}
  assert(battle_hp==0&&battle_hero_hp==hp-predicted);
  advance_battle(now+12);assert(mode==PLAY&&g.hp==hp-predicted&&g.x==5);
  assert(g.gold==gold+e.gold&&g.exp==exp+e.exp&&g.map[1][60]==0);
  finish_battle();assert(g.gold==gold+e.gold);count++;
 }
 new_game();g.floor=1;g.x=4;g.y=5;g.map[1][60]=201;g.hp=10000;g.atk=20;
 {int hp=g.hp,loss=damage(201);input_now=0;walk(1,0);assert(mode==BATTLE);action(5);assert(mode==PLAY&&g.hp==hp-loss);}
 new_game();g.floor=1;g.x=4;g.y=5;g.map[1][60]=201;g.atk=enemy(201).def;walk(1,0);assert(mode==PLAY&&g.map[1][60]==201);
 g.atk=enemy(201).def+1;g.def=0;g.hp=1;walk(1,0);assert(mode==PLAY&&g.hp==1&&g.map[1][60]==201);
 printf("PASS battle: %d enemy/scaling cases match core, skip, guards, input lock and single rewards\n",count);
 return 0;
}
