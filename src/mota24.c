#include "bda_sdk.h"
#include "core.c"
#include "font.h"
#include "sprites.h"
#include "story.h"
/* GCC emits this for structure copies in freestanding builds. */
void *memcpy(void *dst,const void *src,unsigned int n){unsigned char *d=dst;const unsigned char *s=src;while(n--)*d++=*s++;return dst;}

static u8 screen[24+240*320*2];
static bda_handle_t frame,draw,back,owner;
static void *brush;
static volatile int dirty,detached,touch_active,touch_pending;
static volatile u32 touch_value;
static u32 previous,resume,second_tick;
static int touch_escape_suppressed,escape_pending;
static u32 escape_due;
static u32 repeat_direction,repeat_due;
/* 25 ms ticks: first repeat after 300 ms, then one step every 125 ms. */
static u32 direction_repeat(u32 current,u32 now,int enabled){
    u32 direction=current&15u;
    if(!enabled||!direction||(direction&(direction-1u))){repeat_direction=0;return 0;}
    if(direction!=repeat_direction){repeat_direction=direction;repeat_due=now+12;return 0;}
    if((s32)(now-repeat_due)>=0){repeat_due=now+5;return direction;}
    return 0;
}
/* docs/gam4980_sdk_optimization.md: touch also raises packet Escape.
 * Match gam4980_payload.c: release the latch only after a clean low sample. */
static void suppress_touch_escape(void){touch_escape_suppressed=1;escape_pending=0;previous&=~(1u<<4);}
static u32 input_edges(u32 current,u32 now){
    const u32 esc=1u<<4;u32 pressed;
    if(touch_escape_suppressed){
        if(!touch_active&&!(current&esc))touch_escape_suppressed=0;
        current&=~esc;escape_pending=0;
    }
    pressed=current&~previous;previous=current;
    /* Allow queued touch-prefix/coordinate messages to identify an ESC that
     * appeared in the hardware packet just before its window message. */
    if(pressed&esc){escape_pending=1;escape_due=now+4;}
    pressed&=~esc;
    if(escape_pending&&(s32)(now-escape_due)>=0){escape_pending=0;pressed|=esc;}
    return pressed;
}
static Game save_buffer;
static const char savepath[]="A:\\MOTA24.SAV";
static const char backup[]="A:\\MOTA24.BAK";
static u16 color(int r,int g,int b){u16 c=(u16)(((r&248)<<8)|((g&252)<<3)|(b>>3));return c?c:1;}
static void pixel(int x,int y,u16 c){int i;if(x<0||x>=240||y<0||y>=320)return;i=24+(y*240+x)*2;screen[i]=(u8)c;screen[i+1]=(u8)(c>>8);}
static void rect(int x,int y,int w,int h,u16 c){int a,b;for(b=y;b<y+h;b++)for(a=x;a<x+w;a++)pixel(a,b,c);}
static void border(int x,int y,int w,int h,u16 c){rect(x,y,w,1,c);rect(x,y+h-1,w,1,c);rect(x,y,1,h,c);rect(x+w-1,y,1,h,c);}
#define WHITE 0xef7d
#define GOLD 0xf66b
#define MUTED 0x8493
#define DARK 0x1084
#define BLUE 0x4d5d
#define RED 0xf28c
static void text(int x,int y,const char *s,u16 c){
    while(*s){unsigned int ch=(u8)*s++;int lo=0,hi=(int)(sizeof(glyphs)/sizeof(glyphs[0]))-1,j,k;
        if(ch>=224){ch=((ch&15)<<12)|(((u8)s[0]&63)<<6)|((u8)s[1]&63);s+=2;}
        else if(ch>=192){ch=((ch&31)<<6)|((u8)*s++&63);}
        while(lo<=hi){int m=(lo+hi)/2;if(glyphs[m].code<ch)lo=m+1;else if(glyphs[m].code>ch)hi=m-1;else {for(j=0;j<12;j++)for(k=0;k<12;k++)if(glyphs[m].rows[j]&(1<<(11-k)))pixel(x+k,y+j,c);break;}}
        x+=ch<128?6:12;
    }
}
static void number(int x,int y,int v,u16 c){char b[16];int i=14;if(v<0){text(x,y,"无法破防",RED);return;}b[15]=0;do{b[i--]=(char)('0'+v%10);v/=10;}while(v&&i>=0);text(x,y,b+i+1,c);}
static void right_number(int right,int y,int v,u16 c){int n=v,digits=1;while(n>=10){n/=10;digits++;}number(right-digits*6,y,v,c);}
static void key_icon(int x,int y,int kind){int a,b;for(b=0;b<14;b++)for(a=0;a<14;a++){u16 c=key_icons[kind][b*14+a];if(c)pixel(x+a,y+b,c);}}
static void message_text(const char *s){int x=8,y=279;char b[4];while(s&&*s){int n=(u8)*s>=224?3:(u8)*s>=192?2:1,w=n==1?6:12,i;if(x+w>232){x=8;y+=12;}if(y>291)break;for(i=0;i<n;i++)b[i]=*s++;b[n]=0;text(x,y,b,GOLD);x+=w;}}
static int animation_frame,hero_direction;
static int story_page,help_return;
static u32 animation_tick;
static int hero_frame,hero_stride;
static u32 input_now,hero_step_tick;
static const StoryPage *dialog_pages;
static int dialog_page,dialog_count;
static void begin_dialog(const StoryPage *pages,int count){
    dialog_pages=pages;dialog_count=count;dialog_page=0;mode=DIALOG;hero_frame=0;repeat_direction=0;
}
/* Presentation advances independently; the deterministic core commits once. */
static Enemy battle_enemy;
static int battle_id,battle_dx,battle_dy,battle_hp,battle_hero_hp;
static int battle_phase,battle_round,battle_last,battle_amount,battle_special;
static u32 battle_due;
static void finish_battle(void){
    if(mode!=BATTLE)return;
    mode=PLAY;move(battle_dx,battle_dy);hero_frame=0;repeat_direction=0;
    if(mode==WIN)story_page=0;
    dirty=1;
}
static int begin_battle(int dx,int dy,int id){
    int loss=damage(id);
    if(loss<0||loss>=g.hp)return 0;
    battle_enemy=enemy(id);battle_id=id;battle_dx=dx;battle_dy=dy;
    battle_hp=battle_enemy.hp;battle_hero_hp=g.hp;
    battle_special=battle_enemy.special==22?battle_enemy.extra:battle_enemy.special==11?g.hp/4:0;
    battle_phase=battle_special?0:1;battle_round=0;battle_last=3;battle_amount=0;
    battle_due=input_now+6;mode=BATTLE;repeat_direction=0;hero_frame=0;dirty=1;return 1;
}
static void advance_battle(u32 now){
    if(mode!=BATTLE||(s32)(now-battle_due)<0)return;
    dirty=1;battle_due=now+6;
    if(battle_phase==0){battle_hero_hp-=battle_special;battle_amount=battle_special;battle_last=2;battle_phase=1;}
    else if(battle_phase==1){
        int hit=g.atk-battle_enemy.def;
        battle_round++;battle_amount=hit<battle_hp?hit:battle_hp;
        battle_hp-=battle_amount;battle_last=0;
        battle_phase=battle_hp?2:3;if(!battle_hp)battle_due=now+12;
    }else if(battle_phase==2){
        battle_amount=max(0,battle_enemy.atk-g.def);battle_hero_hp-=battle_amount;
        battle_last=1;battle_phase=1;
    }else finish_battle();
}
static void walk(int dx,int dy){
    int x=g.x,y=g.y,floor=g.floor,t=tile(g.floor,g.x+dx,g.y+dy);u32 flags=g.flags;
    if(mode==PLAY&&t>=201&&t<300&&enemy_base[t].hp&&
       !(floor==20&&x+dx==5&&y+dy==7)&&
       !(floor==18&&x+dx==10&&y+dy==10&&!(g.flags&PRINCESS))&&
       !(floor==21&&x+dx==5&&y+dy==0&&!(g.flags&BOSS21))&&begin_battle(dx,dy,t))return;
    move(dx,dy);if(mode==WIN)story_page=0;
    if(mode==PLAY&&t==124){
        if(floor==0){
            if(!(flags&INTRO))begin_dialog(fairy_first,PAGE_COUNT(fairy_first));
            else if(flags&ICE)begin_dialog(fairy_ice,PAGE_COUNT(fairy_ice));
            else if(flags&CROSS)begin_dialog(fairy_bless,PAGE_COUNT(fairy_bless));
            else begin_dialog(fairy_reminder,PAGE_COUNT(fairy_reminder));
        }else if(floor==22){
            if((flags&(FIRE|HEART))==(FIRE|HEART))begin_dialog(fairy_unseal,PAGE_COUNT(fairy_unseal));
            else begin_dialog(fairy_hidden,PAGE_COUNT(fairy_hidden));
        }
    }
    if(g.x!=x||g.y!=y||g.floor!=floor){hero_stride^=1;hero_frame=hero_stride?1:3;hero_step_tick=input_now;}
}
static void animate_hero(u32 now){
    if(hero_frame&&now-hero_step_tick>=5){hero_frame=0;dirty=1;}
}
static void sprite(int x,int y,int t){
    int a,b,frame=t>=300?0:animation_frame;
    const unsigned short *pixels=t>=300?hero_pixels[t-300][hero_frame]:sprite_pixels[sprite_index[t]*2+frame];
    for(b=0;b<20;b++)for(a=0;a<20;a++)pixel(x+a,y+b,pixels[b*20+a]);
}

static const char *title_items[]={"开始冒险","继续存档","操作说明","关于","退出游戏"};
static const char *menu_items[]={"继续冒险","怪物手册","楼层传送","保存进度","读取进度","操作说明","重新开始","保存回标题","保存并退出"};
static const char *shop_lines[6][3]={
    {"25金币  生命 +800","25金币  攻击 +4","25金币  防御 +4"},
    {"100金币 生命 +4000","100金币 攻击 +20","100金币 防御 +20"},
    {"100经验 升一级","30经验  攻击 +5","30经验  防御 +5"},
    {"270经验 升三级","95经验  攻击 +17","95经验  防御 +17"},
    {"10金币  黄钥匙","50金币  蓝钥匙","100金币 红钥匙"},
    {"黄钥匙 卖7金币","蓝钥匙 卖35金币","红钥匙 卖70金币"}};
static void panel(const char *title){rect(14,66,212,211,DARK);border(14,66,212,211,GOLD);text(26,76,title,GOLD);}
/* Dedicated 240x320 title and story pages, using the same pixel assets. */
static void story_backdrop(const char *heading){
    int x,y;rect(0,0,240,320,DARK);border(7,7,226,306,0x52aa);
    border(10,10,220,300,GOLD);text(84,25,"魔塔二十四层",GOLD);
    for(y=0;y<4;y++)for(x=0;x<5;x++)sprite(70+x*20,51+y*20,1);
    for(x=0;x<5;x+=2)sprite(70+x*20,41,1);
    sprite(110,111,85);sprite(110,131,300);
    text(24,160,heading,GOLD);
}
static void render_story(void){
    int i;
    if(mode==TITLE){
        story_backdrop("经典冒险 · 勇者启程");
        for(i=0;i<5;i++){
            if(selection==i)rect(38,184+i*18,164,18,color(57,75,94));
            text(i==3?108:96,187+i*18,title_items[i],selection==i?GOLD:WHITE);
        }
        text(24,286,"方向选择  确定进入",MUTED);
        if(notice&&notice[0])text(24,272,notice,RED);
    }else if(mode==OPENING){
        const StoryPage *p=&opening_pages[story_page];
        story_backdrop(p->speaker);
        if(p->portrait)sprite(190,153,p->portrait);
        text(24,188,p->line1,WHITE);text(24,210,p->line2,WHITE);text(24,232,p->line3,WHITE);
        number(186,25,story_page+1,MUTED);text(195,25,"/8",MUTED);
        text(24,276,story_page==PAGE_COUNT(opening_pages)-1?"确定 / 点击：进入魔塔":"确定 / 点击：继续",GOLD);
        text(24,294,"返回：跳过片头",MUTED);
    }else{
        story_backdrop(g.floor==26?"终章 · 魔塔的黎明":"终章 · 勇者的凯旋");
        if(!story_page){
            text(24,188,"魔王倒下，塔内恢复了宁静。",WHITE);
            text(24,210,g.floor==26?"隐藏的旅程，终于迎来终点。":"你的勇气，为王国带回希望。",WHITE);
            text(24,232,"晨光照亮了勇者归来的道路。",WHITE);
            text(24,276,"确定 / 点击：冒险记录",GOLD);
        }else{
            text(24,188,"最终生命",MUTED);right_number(210,188,g.hp,WHITE);
            text(24,208,"攻击 / 防御",MUTED);right_number(163,208,g.atk,WHITE);right_number(210,208,g.def,WHITE);
            text(24,228,"冒险用时（分钟）",MUTED);right_number(210,228,g.seconds/60,GOLD);
            text(24,252,"感谢游玩，愿勇气与你同在。",WHITE);
            text(24,282,"确定 / 点击：返回主菜单",GOLD);
        }
    }
}
static void render_help(void){panel("操作说明");text(26,103,"方向键：移动或选择",WHITE);text(26,126,"确定键：菜单或确认",WHITE);text(26,149,"返回键：关闭当前页面",WHITE);text(26,172,"点相邻格：移动或交互",WHITE);text(26,195,"底部按钮：手册菜单传送",WHITE);text(26,218,"菜单中手动保存和读取",GOLD);text(26,241,"隐藏：25分钟内到16层",MUTED);}
static void render(void){
    int x,y,i,n=0,seen[300];
    if(mode==ABOUT){
        rect(0,0,240,320,DARK);border(7,7,226,306,0x52aa);border(10,10,220,300,GOLD);
        text(108,30,"关于",GOLD);text(84,64,"魔塔二十四层",WHITE);
        text(28,108,"作者",MUTED);text(88,108,"HelloClyde",WHITE);
        text(28,156,"感谢：",GOLD);text(28,181,"QQ 群",MUTED);
        text(28,204,"「步步高电子词典游戏群」",WHITE);
        text(28,233,"群号：830340878",GOLD);
        text(54,289,"确定 / 返回：主菜单",MUTED);return;
    }
    if(mode==HELP&&help_return==TITLE){
        rect(0,0,240,320,DARK);border(7,7,226,306,0x52aa);border(10,10,220,300,GOLD);
        text(84,25,"魔塔二十四层",GOLD);render_help();text(54,289,"确定 / 返回：主菜单",MUTED);return;
    }
    if(mode==TITLE||mode==OPENING||mode==WIN){render_story();return;}
    rect(0,0,240,320,DARK);text(8,4,"魔塔",GOLD);right_number(50,4,g.floor==26?24:g.floor>=23?23:g.floor,GOLD);text(54,4,"层",GOLD);
    text(84,4,"生命",MUTED);right_number(168,4,mode==BATTLE?battle_hero_hp:g.hp,WHITE);text(180,4,"等级",MUTED);right_number(232,4,g.level,WHITE);
    text(8,21,"攻",RED);right_number(70,21,g.atk,WHITE);text(84,21,"防",BLUE);right_number(146,21,g.def,WHITE);text(160,21,"金",GOLD);right_number(232,21,g.gold,WHITE);
    key_icon(8,37,0);right_number(44,38,g.keys[0],WHITE);key_icon(54,37,1);right_number(90,38,g.keys[1],WHITE);key_icon(100,37,2);right_number(136,38,g.keys[2],WHITE);text(148,38,"经验",MUTED);right_number(232,38,g.exp,WHITE);
    border(7,53,226,226,0xbdf7);border(8,54,224,224,0x52aa);
    for(y=0;y<11;y++)for(x=0;x<11;x++)sprite(10+x*20,56+y*20,tile(g.floor,x,y));
    sprite(10+g.x*20,56+g.y*20,300+hero_direction);
    message_text(notice);
    rect(8,305,68,15,color(43,58,75));rect(86,305,68,15,color(43,58,75));rect(164,305,68,15,color(43,58,75));
    text(30,306,"手册",WHITE);text(108,306,"菜单",WHITE);text(186,306,"传送",WHITE);
    if(mode==MENU){panel("冒险菜单");for(i=0;i<9;i++){if(i==selection)rect(22,96+i*18,196,18,color(57,75,94));text(34,99+i*18,menu_items[i],i==selection?GOLD:WHITE);}}
    if(mode==SHOP){panel(shop<2?"金币商店":shop<4?"经验商店":"钥匙商人");for(i=0;i<3;i++){if(i==selection)rect(22,114+i*34,196,29,color(57,75,94));text(29,122+i*34,shop_lines[shop][i],WHITE);}text(26,240,"确定购买 / 返回离开",MUTED);}
    if(mode==BOOK){
        panel("怪物手册  左右翻页");for(i=0;i<300;i++)seen[i]=0;
        for(i=0;i<121;i++){int t=g.map[g.floor][i];if(t>=201&&t<300&&enemy_base[t].hp)seen[t]=1;}
        for(i=201;i<300;i++)if(seen[i]){if(n/3==bookpage){Enemy e=enemy(i);int yy=99+(n%3)*55,d=damage(i);sprite(24,yy,i);text(49,yy,e.name,WHITE);text(49,yy+14,"生命",MUTED);number(76,yy+14,e.hp,WHITE);text(133,yy+14,"损",MUTED);number(148,yy+14,d,d<0||d>=g.hp?RED:GOLD);text(49,yy+28,"攻",MUTED);number(64,yy+28,e.atk,WHITE);text(122,yy+28,"防",MUTED);number(138,yy+28,e.def,WHITE);}n++;}
        if(!n)text(30,126,"本层没有怪物",WHITE);
        text(26,259,"页",MUTED);number(43,259,bookpage+1,WHITE);text(65,259,"返回关闭",MUTED);
    }
    if(mode==FLY){panel("风之罗盘");text(28,110,"上下选择已到达楼层",WHITE);text(28,145,"前往楼层",GOLD);number(142,145,selection,GOLD);text(28,190,"确定传送 / 返回关闭",MUTED);}
    if(mode==RESTART){panel("重新开始？");text(28,118,"当前未保存进度将丢失",WHITE);text(28,155,"确定开始新冒险",GOLD);text(28,188,"返回取消",MUTED);}
    if(mode==WIN){panel("冒险完成");text(28,116,g.floor==26?"二十四层：最终魔王已败":"二十一层：冥灵魔王已败",GOLD);text(28,155,"感谢勇士拯救魔塔",WHITE);text(28,194,"确定返回菜单",MUTED);}
    if(mode==HELP)render_help();
    if(mode==BATTLE){
        int hw=battle_hero_hp/max(1,g.hp/60),ew=battle_hp/max(1,battle_enemy.hp/60);
        if(hw>60)hw=60;if(ew>60)ew=60;
        panel("战斗中");text(30,100,"勇士",GOLD);text(138,100,battle_enemy.name,WHITE);
        sprite(48+(battle_last==0?6:0),123,300+2);sprite(168-(battle_last==1?6:0),123,battle_id);
        if(battle_last==0)border(164,119,28,28,RED);
        if(battle_last==1||battle_last==2)border(44,119,28,28,RED);
        text(101,128,"VS",GOLD);
        border(29,155,62,7,MUTED);rect(30,156,hw,5,RED);
        border(149,155,62,7,MUTED);rect(150,156,ew,5,RED);
        right_number(96,171,battle_hero_hp,WHITE);right_number(216,171,battle_hp,WHITE);
        text(30,199,"回合",MUTED);number(64,199,battle_round,WHITE);
        text(30,220,battle_last==3?"准备交战":battle_last==0?"勇士攻击":battle_last==1?"怪物反击":"特殊伤害",GOLD);
        if(battle_last!=3){text(140,220,"-",RED);number(152,220,battle_amount,RED);}
        if(!battle_hp){text(30,244,"胜利！金币",GOLD);number(100,244,battle_enemy.gold,WHITE);text(137,244,"经验",GOLD);number(164,244,battle_enemy.exp,WHITE);}
        else text(30,251,"确定：快速结束战斗",MUTED);
    }
    if(mode==DIALOG){
        const StoryPage *p=&dialog_pages[dialog_page];
        rect(12,155,216,149,DARK);border(12,155,216,149,GOLD);
        sprite(24,166,p->portrait);text(54,169,p->speaker,GOLD);
        right_number(188,169,dialog_page+1,MUTED);text(191,169,"/",MUTED);number(198,169,dialog_count,MUTED);
        text(24,199,p->line1,WHITE);text(24,219,p->line2,WHITE);text(24,239,p->line3,WHITE);
        text(24,278,"确定 / 点击：继续",GOLD);
    }
}
static int read_save(const char *path,Game *out){int f=bda_fs_fopen_raw(path,"rb"),n;if(!bda_fs_file_is_valid(f))return 0;n=bda_fs_read_raw(f,out,sizeof(*out));bda_fs_close_raw(f);return n==(int)sizeof(*out)&&valid_save(out);}
static int write_save(const char *path,const Game *s){int f=bda_fs_fopen_raw(path,"wb"),n;if(!bda_fs_file_is_valid(f))return 0;n=bda_fs_write_raw(f,s,sizeof(*s));bda_fs_flush_all();bda_fs_close_raw(f);return n==(int)sizeof(*s);}
static int save(void){
    if(read_save(savepath,&save_buffer)&&!write_save(backup,&save_buffer)){notice="备份失败，原存档保留";return 0;}
    g.checksum=checksum(&g);
    if(!write_save(savepath,&g)||!read_save(savepath,&save_buffer)){notice="保存失败，请检查存储空间";return 0;}
    notice="进度已保存";return 1;
}
static void load_game(void){if(read_save(savepath,&save_buffer)||read_save(backup,&save_buffer)){g=save_buffer;notice="进度已读取";mode=PLAY;}else notice="没有有效存档";}
static void book(void){if(g.flags&BOOKFLAG){mode=BOOK;bookpage=0;}else notice="先找到一层圣光徽";}
static void fly(void){if((g.flags&FLYFLAG)&&g.floor<21){mode=FLY;selection=g.floor;}else notice="需要风之罗盘，21层起无法传送";}
static void action(int key){
    dirty=1;
    if(mode==BATTLE){if(key==5)finish_battle();return;}
    if(mode==ABOUT){if(key==4||key==5){mode=TITLE;selection=3;}return;}
    if(mode==DIALOG){if(key==5&&++dialog_page>=dialog_count){mode=PLAY;repeat_direction=0;}return;}
    if(mode==TITLE){
        if(key==2)selection=(selection+1)%5;if(key==3)selection=(selection+4)%5;
        if(key==5){if(selection==0){new_game();hero_frame=0;story_page=0;mode=OPENING;}
            else if(selection==1)load_game();
            else if(selection==2){help_return=TITLE;mode=HELP;}
            else if(selection==3)mode=ABOUT;else quit_requested=1;}
        return;
    }
    if(mode==OPENING){if(key==4||(key==5&&++story_page>=PAGE_COUNT(opening_pages))){mode=PLAY;notice="方向键移动，确定打开菜单";}return;}
    if(mode==WIN){if(key==5){if(story_page++==1){mode=TITLE;selection=0;notice="";}}return;}
    if(mode==HELP&&(key==4||key==5)){mode=help_return;return;}
    if(key==4){if(mode==PLAY){mode=MENU;selection=0;}else mode=PLAY;return;}
    if(mode==PLAY){if(key<4){static const int dirs[]={2,1,0,3};hero_direction=dirs[key];}if(key==0)walk(1,0);if(key==1)walk(-1,0);if(key==2)walk(0,1);if(key==3)walk(0,-1);if(key==5){mode=MENU;selection=0;}return;}
    if(mode==MENU){if(key==2)selection=(selection+1)%9;if(key==3)selection=(selection+8)%9;if(key==5){switch(selection){case 0:mode=PLAY;break;case 1:book();break;case 2:fly();break;case 3:save();break;case 4:load_game();break;case 5:help_return=PLAY;mode=HELP;break;case 6:mode=RESTART;break;case 7:if(save()){mode=TITLE;selection=1;notice="";}break;case 8:if(save())quit_requested=1;break;}}return;}
    if(mode==SHOP){if(key==2)selection=(selection+1)%3;if(key==3)selection=(selection+2)%3;if(key==5)buy(selection);return;}
    if(mode==BOOK){int seen[300],i,n=0;for(i=0;i<300;i++)seen[i]=0;for(i=0;i<121;i++){int t=g.map[g.floor][i];if(t>=201&&t<300&&enemy_base[t].hp)seen[t]=1;}for(i=201;i<300;i++)n+=seen[i];if((key==0||key==2)&&(bookpage+1)*3<n)bookpage++;if((key==1||key==3)&&bookpage>0)bookpage--;if(key==5)mode=PLAY;return;}
    if(mode==FLY){if(key==2||key==3){int d=key==2?1:-1,i=selection+d;while(i>=0&&i<21){if(g.visited&(1u<<i)){selection=i;break;}i+=d;}}if(key==5){int i;for(i=0;i<121;i++)if(g.map[selection][i]==88||g.map[selection][i]==87)break;if(i<121){arrive(selection,i%11,i/11);mode=PLAY;}}return;}
    if(key==5){if(mode==RESTART){new_game();story_page=0;mode=OPENING;}else if(mode==WIN){mode=MENU;selection=0;}else mode=PLAY;}
}
static void touch(u32 packed){int x=packed&65535,y=packed>>16;
    if(mode==BATTLE){if(x>=14&&x<226&&y>=240&&y<277)action(5);}
    else if(mode==DIALOG){if(x>=12&&x<228&&y>=155&&y<304)action(5);}
    else if(mode==TITLE){if(x>=38&&x<202&&y>=184&&y<274){selection=(y-184)/18;action(5);}}
    else if(mode==OPENING||mode==WIN){if(y>=176)action(5);}
    else if(mode==PLAY){if(y>=305){if(x<80)book();else if(x<160){mode=MENU;selection=0;}else fly();}
        else if(x>=10&&x<230&&y>=56&&y<276){int dx=(x-10)/20-g.x,dy=(y-56)/20-g.y;if(dx*dx+dy*dy==1){hero_direction=dy<0?3:dy>0?0:dx<0?1:2;walk(dx,dy);}}}
    else if(mode==MENU){if(x>=22&&x<218&&y>=96&&y<258){selection=(y-96)/18;action(5);}}
    else if(mode==SHOP&&x>=22&&x<218&&y>=114&&y<216){selection=(y-114)/34;action(5);}
    else if(mode==BOOK){if(y>245)action(4);else action(x<120?1:0);}
    else if(mode==FLY){if(y>230)action(4);else if(y>175)action(5);else action(x<120?3:2);}
    else action(4);dirty=1;
}
static void release_draw(void){if(draw&&(s32)draw!=-1)bda_gui_end_draw(draw);draw=0;owner=0;}
static void acquire(bda_handle_t h){if(draw&&owner==h)return;release_draw();draw=bda_gui_current_draw(h);if((s32)draw==-1)draw=0;owner=h;}
static int proc(bda_handle_t h,u32 msg,u32 wp,u32 lp){
    if(msg==BDA_MSG_DRAW_CONTEXT_ATTACH){acquire(h);dirty=1;}
    if(msg==BDA_MSG_DRAW_CONTEXT_DETACH){release_draw();detached=1;}
    if(msg==BDA_MSG_REDRAW_INPUT)dirty=1;
    if(msg==0x21){suppress_touch_escape();resume=bda_gui_tick_count_25ms()+4;}
    if(msg==BDA_MSG_TOUCH_COORDINATE){touch_active=1;suppress_touch_escape();resume=bda_gui_tick_count_25ms()+2;return 1;}
    if(msg==BDA_MSG_TOUCH_RELEASE){touch_active=0;suppress_touch_escape();resume=bda_gui_tick_count_25ms()+2;touch_value=lp;touch_pending=1;return 1;}
    return bda_gui_default_proc(h,msg,wp,lp);
}
static int present(void){void *old;int r;render();r=bda_gui_draw_vx(back,0,0,screen);bda_gui_draw_guard_begin();old=bda_gui_select_draw_object(draw,brush);r|=bda_gui_context_copy(back,0,0,240,320,draw,0,0,0);bda_gui_select_draw_object(draw,old);bda_gui_draw_guard_end();dirty=0;return r;}
__attribute__((section(".text.bda_main")))
int bda_main(void){
    bda_frame_desc_t d;bda_gui_message_t m;int i,closing=0,wait=0;
    bda_memset(screen,0,sizeof(screen));screen[0]='V';screen[1]='X';for(i=2;i<6;i++)screen[i]=204;screen[6]=240;screen[10]=64;screen[11]=1;for(i=14;i<20;i++)screen[i]=204;for(i=20;i<24;i++)screen[i]=255;
    bda_memset(&d,0,sizeof(d));bda_memset(&m,0,sizeof(m));frame=draw=back=owner=0;dirty=1;detached=touch_active=touch_pending=0;quit_requested=0;previous=resume=0;touch_escape_suppressed=escape_pending=0;
    repeat_direction=repeat_due=0;hero_frame=hero_stride=0;input_now=hero_step_tick=0;
    new_game();mode=TITLE;selection=story_page=0;notice="";help_return=PLAY;hero_direction=3;animation_frame=0;animation_tick=0;d.title="MOTA24";d.wndproc=proc;d.height=240;d.width=320;
    frame=bda_gui_register_frame_desc(&d);if(!frame||(s32)frame==-1)return 1;
    bda_gui_frame_activate(frame,0x100);acquire(frame);brush=bda_gui_draw_object_create(7);
    if(!draw||!brush||(s32)(u32)brush==-1)goto cleanup;
    back=bda_gui_compatible_context_create(draw);if(!back||(s32)back==-1){back=0;goto cleanup;}
    second_tick=bda_gui_tick_count_25ms();present();
    while(!detached){
        int pump=bda_gui_event_pump_frame_once(&m,frame);u32 now=bda_gui_tick_count_25ms();
        if(!closing){
            input_now=now;animate_hero(now);advance_battle(now);
            if(touch_pending){u32 v=touch_value;touch_pending=0;touch(v);}
            {bda_gui_input_packet_t p;u32 cur=0,pressed;bda_gui_input_packet(&p);for(i=0;i<6;i++)if(p.bytes[i]==1)cur|=1u<<i;
             pressed=input_edges(cur,now);
             pressed|=direction_repeat(cur,now,mode==PLAY&&!touch_active&&(s32)(now-resume)>=0);
             if(!touch_active&&(s32)(now-resume)>=0)for(i=0;i<6;i++)if(pressed&(1u<<i)){action(i);break;}}
            if(now-second_tick>=40){if(mode!=TITLE&&mode!=OPENING&&mode!=WIN&&mode!=DIALOG&&mode!=ABOUT)g.seconds+=(now-second_tick)/40;second_tick+=((now-second_tick)/40)*40;}
            if(now-animation_tick>=16){animation_tick=now;animation_frame^=1;if(mode==PLAY)dirty=1;}
            if(dirty&&draw)present();
            if(quit_requested){bda_gui_frame_stop(frame);bda_gui_frame_release(frame);closing=1;}
        }else if(!pump||++wait>=128)break;
        bda_sys_delay(1);
    }
cleanup:
    if(!closing){bda_gui_frame_stop(frame);bda_gui_frame_release(frame);}
    if(back)bda_gui_compatible_context_free(back);release_draw();bda_gui_close_frame(frame);return 0;
}
