/* Portable deterministic game rules; also compiled by the host test suite. */
#include "data.h"
typedef struct {
    unsigned int magic,version,checksum;
    int floor,x,y,hp,atk,def,gold,exp,level,keys[3];
    unsigned int visited,flags,seconds;
    unsigned short map[FLOORS][121];
} Game;
static Game g;
static int mode,selection,shop,bookpage,quit_requested;
static const char *notice;
enum { PLAY, MENU, BOOK, FLY, SHOP, RESTART, WIN, HELP, TITLE, OPENING, DIALOG, ABOUT };
enum { INTRO=1,BOOKFLAG=2,FLYFLAG=4,CROSS=8,HAMMER=16,THIEF=32,ICE=64,SECRET=128,BLESS=256,PRINCESS=512,BOSS16=1024,BOSS19=2048,BOSS21=4096,FIRE=8192,HEART=16384,BLOOD=32768 };
static int max(int a,int b){return a>b?a:b;}
static int tile(int f,int x,int y){return x<0||x>10||y<0||y>10?1:g.map[f][y*11+x];}
static void set(int f,int x,int y,int v){g.map[f][y*11+x]=(unsigned short)v;}
static void new_game(void){
    int f,i; unsigned char *p=(unsigned char*)&g;
    for(i=0;i<(int)sizeof(g);i++)p[i]=0;
    g.magic=0x4d543234;g.version=1;g.x=5;g.y=9;g.hp=1000;g.atk=g.def=10;g.level=1;g.visited=1;
    for(f=0;f<FLOORS;f++)for(i=0;i<121;i++)g.map[f][i]=initial_maps[f][i];
    mode=PLAY;selection=0;notice="方向键移动，确定打开菜单";
}
static Enemy enemy(int id){
    Enemy e=enemy_base[id];
    if(g.flags&BOSS16){
        if(id==228){e.hp=1600;e.atk=1306;e.def=1200;e.gold=117;e.exp=100;}
        if(id==212){e.hp=3333;e.atk=1200;e.def=1133;e.gold=112;e.exp=100;}
        if(id==247){e.hp=2000;e.atk=1106;e.def=973;e.gold=106;e.exp=93;}
        if(id==245){e.hp=20000;e.atk=e.def=1333;e.gold=e.exp=133;}
    }
    if((g.flags&BOSS19)&&id==208){e.hp=45000;e.atk=2550;e.def=2250;e.gold=312;e.exp=275;}
    if(g.flags&BOSS21){
        if(id==228){e.hp=2400;e.atk=2612;e.def=2400;e.gold=146;e.exp=125;}
        if(id==212){e.hp=4999;e.atk=2400;e.def=2266;e.gold=140;e.exp=125;}
        if(id==247){e.hp=3000;e.atk=2212;e.def=1946;e.gold=132;e.exp=116;}
        if(id==245){e.hp=30000;e.atk=e.def=2666;e.gold=e.exp=166;}
        if(id==208){e.hp=60000;e.atk=3400;e.def=3000;e.gold=390;e.exp=343;}
    }
    return e;
}
static int damage(int id){
    Enemy e=enemy(id); int hit=g.atk-e.def,turns,loss;
    if(hit<=0)return -1;
    turns=(e.hp+hit-1)/hit-1;
    loss=max(0,turns)*max(0,e.atk-g.def);
    if(e.special==22)loss+=e.extra;
    if(e.special==11)loss+=g.hp/4;
    return loss;
}
static void arrive(int f,int x,int y){g.floor=f;g.x=x;g.y=y;g.visited|=1u<<f;notice="谨慎分配钥匙，先看战斗损血";}
static void open_shop(int s){shop=s;selection=0;mode=SHOP;notice="上下选择，确定交易，返回离开";}
static int npc(int id,int x,int y){
    int f=g.floor;
    if(id==131){open_shop(f==3?0:1);return 0;}
    if(f==5&&x==1&&y==7){open_shop(2);return 0;}
    if(f==13){open_shop(3);return 0;}
    if(f==5){open_shop(4);return 0;}
    if(f==12){open_shop(5);return 0;}
    if(f==0){
        if(!(g.flags&INTRO)){
            g.flags|=INTRO;g.keys[0]++;g.keys[1]++;g.keys[2]++;set(0,4,8,124);set(0,5,8,0);
            notice="仙子：带回七层十字架。赠三色钥匙";return 0;
        }
        if(g.flags&ICE){g.flags=(g.flags&~ICE)|SECRET;notice="冰杖开启隐藏剧情，再带回十字架";return 0;}
        if(g.flags&CROSS){g.flags=(g.flags&~CROSS)|BLESS;g.hp=g.hp*4/3;g.atk=g.atk*4/3;g.def=g.def*4/3;notice="仙子祝福！全能力提升三分之一";return 1;}
        notice="仙子：十字架在七层，宝剑在三层";return 0;
    }
    if(f==2){if(id==121)g.atk+=70;else g.def+=30;notice="获救者赠予装备";return 1;}
    if(id==123){
        if(!(g.flags&THIEF)){g.flags|=THIEF;set(2,1,6,0);notice="小偷打开二层铁门，正在寻找神锒";return 0;}
        if(g.flags&HAMMER){g.flags&=~HAMMER;set(18,5,8,0);set(18,5,9,0);notice="十八层通路已修复";return 1;}
        notice="把十二层星光神锒带给我";return 0;
    }
    if(f==15){
        int *currency=x==4?&g.exp:&g.gold;
        if(*currency<500){notice=x==4?"需要500经验换圣剑":"需要500金币换圣盾";return 0;}
        *currency-=500;if(x==4)g.atk+=120;else g.def+=120;notice="获得神圣装备";return 1;
    }
    if(f==16){
        if(g.seconds>=1500||(g.flags&(CROSS|BLESS))){notice="神秘老人已离开";return 1;}
        g.flags|=ICE;notice="获得冰之灵杖，先返回序章找仙子";return 1;
    }
    if(id==132){g.flags|=PRINCESS;notice="公主获救，十八层上行楼梯已开启";return 0;}
    if(f==22){
        if((g.flags&(FIRE|HEART))==(FIRE|HEART)){g.flags|=BLOOD;set(26,5,3,258);notice="三杖齐聚，地下血影封印解除";return 1;}
        notice="寻找二十三层东西两处的灵杖";return 0;
    }
    notice="前路危险，善用手册与存档";return 0;
}
static void move(int dx,int dy){
    int x=g.x+dx,y=g.y+dy,f=g.floor,t=tile(f,x,y),i,loss;
    if(mode!=PLAY)return;
    if(t==1||t==3||t==4||t==5||t==7||t==8||(t>=181&&t<=196))return;
    if(f==20&&x==5&&y==7){
        if(!(g.flags&BLESS)){notice="需要仙子祝福才能前往二十一层";return;}
        arrive(21,5,5);return;
    }
    if(f==18&&x==10&&y==10&&!(g.flags&PRINCESS)){notice="先救出公主";return;}
    if(f==21&&x==5&&y==0&&!(g.flags&BOSS21)){notice="先击败本层魔王";return;}
    if(t==86){notice="铁门封闭，请寻找另一条路";return;}
    if(t==85){
        if(f==25&&(g.flags&(FIRE|HEART))!=(FIRE|HEART)){notice="需要炎之灵杖和心之灵杖";return;}
        if(f!=25&&!(g.flags&BLESS)){notice="需要仙子祝福";return;}
        set(f,x,y,0);
    }
    if(t>=81&&t<=83){if(g.keys[t-81]<=0){notice="缺少对应颜色钥匙";return;}g.keys[t-81]--;set(f,x,y,0);notice="门已打开";}
    if(t>=201&&t<300&&enemy_base[t].hp){
        loss=damage(t);if(loss<0){notice="无法破防，先提升攻击";return;}
        if(loss>=g.hp){notice="生命不足，请提升能力或绕行";return;}
        {Enemy e=enemy(t);g.hp-=loss;g.gold+=e.gold;g.exp+=e.exp;}
        set(f,x,y,0);notice="战斗胜利，获得金币与经验";
        if(f==16&&x==5&&y==5)g.flags|=BOSS16;
        if(f==19&&x==5&&y==6)g.flags|=BOSS19;
        if(f==21&&x==5&&y==1){g.flags|=BOSS21;set(21,5,6,0);if(g.flags&SECRET)set(21,5,0,87);else {mode=WIN;notice="二十一层结局：击败冥灵魔王";}}
        if(f==26&&(t==257||t==258)){mode=WIN;notice=t==258?"二十四层结局：血影已被击败":"二十四层结局：魔龙已被击败";}
    }
    if((t>=121&&t<=124)||t==131||t==132){if(npc(t,x,y))set(f,x,y,0);return;}
    if(t>=21&&t<=73){
        if(t<=23)g.keys[t-21]++;
        if(t==26)for(i=0;i<3;i++)g.keys[i]++;
        if(t==27)g.atk+=3;
        if(t==28)g.def+=3;
        if(t==31)g.hp+=200;
        if(t==32)g.hp+=500;
        if(t>=35&&t<=44){static const int a[]={10,10,70,30,70,85,120,120,150,150};if(t&1)g.atk+=a[t-35];else g.def+=a[t-35];}
        if(t==45)g.flags|=BOOKFLAG;
        if(t==46)g.flags|=FLYFLAG;
        if(t==48)g.flags|=HAMMER;
        if(t==50){g.level++;g.hp+=1000;g.atk+=10;g.def+=10;}
        if(t==55){g.flags|=CROSS;set(16,4,4,1);}
        if(t==56)g.hp*=2;
        if(t==63)g.gold+=300;
        if(t==72)g.flags|=FIRE;
        if(t==73)g.flags|=HEART;
        set(f,x,y,0);notice="获得宝物，能力已更新";
        if(t==45)notice="获得圣光徽，可在菜单查看怪物手册";
        if(t==46)notice="获得风之罗盘，可传送到已到达楼层";
        if(t==55)notice="获得十字架，带回序章交给仙子";
        if(t==48)notice="获得星光神锒，带给四层小偷";
    }
    if(t==125){g.level+=3;g.hp+=3000;g.atk+=30;g.def+=30;set(f,x,y,0);notice="大飞羽：提升三级";}
    if(t==6)set(f,x,y,0);
    for(i=0;i<(int)(sizeof(stairs)/sizeof(stairs[0]));i++){
        const Stair *s=&stairs[i];
        if(s->floor==f&&s->x==x&&s->y==y){
            if(f==25&&s->to==26&&(g.flags&(FIRE|HEART))!=(FIRE|HEART)){notice="收集两支灵杖后进入地下";return;}
            arrive(s->to,s->tx,s->ty);return;
        }
    }
    g.x=x;g.y=y;
}
static int buy(int n){
    int cost,boost,*money=&g.gold;
    if(n<0||n>2)return 0;
    if(shop<2){cost=shop?100:25;boost=shop?20:4;}
    else if(shop<4){money=&g.exp;cost=n?(shop==2?30:95):(shop==2?100:270);boost=shop==2?5:17;}
    else {static const int prices[]={10,50,100};cost=prices[n];boost=0;}
    if(shop==5){static const int sale[]={7,35,70};if(!g.keys[n]){notice="没有可出售的钥匙";return 0;}g.keys[n]--;g.gold+=sale[n];notice="钥匙已出售";return 1;}
    if(*money<cost){notice="金币或经验不足";return 0;}
    *money-=cost;
    if(shop==4)g.keys[n]++;
    else if(n==1)g.atk+=boost;
    else if(n==2)g.def+=boost;
    else if(shop<2)g.hp+=shop?4000:800;
    else {int b=shop==2?1:3;g.level+=b;g.hp+=1000*b;g.atk+=shop==2?7:20;g.def+=shop==2?7:20;}
    notice="交易成功";return 1;
}
static unsigned int checksum(const Game *s){
    const unsigned char *p=(const unsigned char*)s;unsigned int h=2166136261u;int i;
    for(i=12;i<(int)sizeof(*s);i++){h^=p[i];h*=16777619u;}return h;
}
static int valid_save(const Game *s){
    int f,i;
    if(s->magic!=0x4d543234||s->version!=1||s->checksum!=checksum(s)||s->floor<0||s->floor>=FLOORS||s->x<0||s->x>10||s->y<0||s->y>10||s->hp<=0||s->hp>100000000||s->atk<=0||s->atk>1000000||s->def<0||s->def>1000000)return 0;
    if(s->gold<0||s->exp<0||s->level<1)return 0;
    for(i=0;i<3;i++)if(s->keys[i]<0||s->keys[i]>1000000)return 0;
    for(f=0;f<FLOORS;f++)for(i=0;i<121;i++)if(s->map[f][i]>=300)return 0;
    return 1;
}
