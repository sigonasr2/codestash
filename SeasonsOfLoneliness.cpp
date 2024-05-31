#define OLC_PGE_APPLICATION
#include "pixelGameEngine.h"
#include "data.h"
#define OLC_PGEX_SPLASHSCREEN
#include "splash.h"
#include <queue>
#define OLC_SOUNDWAVE
#include "soundwaveEngine.h"

using namespace olc;

//#define TEST_MAP1 //Toggle to just play around on map 1.
//#define TEST_MAP2 //Toggle on to just play around on map 2.

#define SKIP_INTRO false
#define SKIP_CHAPTER1 false
#define SKIP_BOSS false
#define SKIP_SILICON2 false
#define SKIP_CHAPTER2 false
#define SKIP_STORYBOOK false
#define SKIP_CHAPTER3 false
#define SKIP_LAUNCHPAD false
#define SKIP_A_A false

#define STARTING_MAP MAP_1
#define STARTING_STATE CUTSCENE_3
#define MOVE_SPD 0.075
#define PLAYER_X 14
#define PLAYER_Y 4

#ifdef TEST_MAP1
	#define STARTING_MAP "assets/maps/map1"
	#define STARTING_STATE GAMEWORLD
	#define MOVE_SPD 0.1
	#define PLAYER_X 40
	#define PLAYER_Y 31
#endif
#ifdef TEST_MAP2
	#define STARTING_MAP "assets/maps/map2"
	#define STARTING_STATE GAMEWORLD
	#define MOVE_SPD 0.1
	#define PLAYER_X 16
	#define PLAYER_Y 5
#endif

enum GAMESTATE{
	CUTSCENE_1,
	CUTSCENE_2,
	CUTSCENE_3,
	GAMEWORLD,
	WAITING_FOR_CUTSCENE_3,
	GAME_OVER,
	GAME_OVER_TERMINAL,
	COLLECTED_SILICON,
	CUTSCENE_4, //First rover repaired.
	CUTSCENE_4_DONE,
	LATER_THAT_NIGHTFADEIN,
	LATER_THAT_NIGHTWAIT,
	LATER_THAT_NIGHTFADEOUT,
	CUTSCENE_5, //Third rover repaired.
	DISPLAY_BOOK,
	DISPLAY_BOOK2,
	CUTSCENE_6,
	IN_SPACE,
	FIN,
	THANKS
};

namespace cutscene{
	enum CUTSCENE{
		NONE,
		PAN_DOME,
		PAUSE_TO_CUTSCENE_3,
		CUTSCENE_4,
		TRANSITION_CUTSCENE, //Exit the dome.
		TRANSITION_CUTSCENE_2, //Enter the dome.
		NODE_COLLECT_CUTSCENE,
		PAN_OVER_TO_CROPS,
		RAINING_IN_DOME,
		AFTER_RAIN,
		WALK_TO_COMPUTER,
		INPUT_USERNAME,
		GO_OUTSIDE,
		WALK_TO_ROVER,
		DISPLAY_VOLCANIC_AREA,
		END_VOLCANIC_AREA_CUTSCENE,
		REPAIR_ROVER_1,
		GET_SOME_REST,
		IN_BED,
		SHAKE,
		INVESTIGATE_X_X,
		CHECK_COORDS,
		SPAWN_WORMS,
		CHECK_COORDS_2,
		MAP_TRANSITION,
		CHECK_COORDS_3,
		STORY_REVIEW,
		STORY_REVIEW2,
		FINAL_REVIEW,
		CHECK_COORDS_4,
		PREPARE_SEQUENCE,
		LAUNCHPAD_OPEN,
		LAUNCHPAD_PREPARE,
		READY_TO_BOARD,
		INTRUDERS_DETECTED,
		A_A_ENCOUNTER,
		A_A_TRANSFORM,
		ROVER_SAVE,
	};
}

namespace battle{
	enum BATTLESTATE{
		NONE,
		WAITING_FOR_CAMERA,
		PLAYER_SELECTION,
		PLAYER_TARGET_SELECTION,
		ENEMY_SELECTION,
		MOVE_RESOLUTION,
		PERFORM_TURN,
		WAIT_TURN_ANIMATION,
		WAIT_TURN_ANIMATION2,
		DAMAGE_RESOLUTION,
	};
}

namespace gameflag{
	enum FLAG{
		TUTORIAL_WALKED_OFF_FARM,
		VISIT_BROKEN_ROVER,
		COLLECTED_SILICON_1,
		REPAIRED_ROVER_1,
		REST_IN_DOME,
		SLEEP,
		FIRST_ENCOUNTER_X_X,
		ANALYSIS_X_X,
		TUTORIAL1_X_X,
		TUTORIAL2_X_X,
		CHECK_ROVER,
		NEXT_COORDS,
		COLLECTED_SILICON_2,
		CHECK_ROVER_2,
		COLLECTED_SILICON_3,
		CHECK_ROVER_3,
		STORY_REVIEW,
		NEXT_COORDS2,
		CHECK_ROVER_4,
		LITTLESNAKES_KILLED,
		COLLECTED_SILICON_4,
		CHECK_ROVER_5,
		ROCKET_LAUNCH_READY,
		COLLECTED_ITEMS_IN_DOME,
		BOARDED_ROCKET,
		INTRUDER_DETECTED,
		DEFEATED_Y_Y,
		DEFEATED_A_A,
	};
}

enum MOVEMENT_PRIORITY{
	HORZ_FIRST,
	VERT_FIRST,
	BOTH
};

class Seed{
	public:
		vf2d pos;
		Seed(vf2d pos) {
			this->pos=pos;
		}
};

namespace reasoncode{
	enum Code{
		SEED_GROWTH,
		TREE_BURN,
		DROUGHT,
		PETRIFIED,
		RADIOACTIVE
	};
}

#define MAX_TERMINAL_NAME_LENGTH 6
#define WIDTH 256
#define HEIGHT 224
#define ALPHA_SCREEN1 128
#define ALPHA_SCREEN2 20
#define FADE_SPD 6
#define MESSAGE_SCROLL_WAIT_SPD 1 //Number of frames to wait.
#define BATTLE_CAMERA_SCROLL_SPD 0.05 //How fast to scroll over to the battle.

class DisplayNumber{
	public:
		int number; //Negative means healing.
		int frame; //Frame it was created.
		float x,y;
		int alpha=255;
		DisplayNumber(int numb,float x,float y,int frameCount) {
			this->number=numb;
			this->x=x;
			this->y=y;
			this->frame=frameCount;
		}
};

class Animation{
	public:
		Decal*spr;
		std::vector<vi2d> frames;
		int frame=0;
		bool flipped=false;
		int skip_frames=1; //Essentially the animation speed. Number of frames to wait for next frame.
		int width=32;
		int height=32;

		vi2d getCurrentFrame() {
			return frames[frame];
		}
};

std::vector<Animation*> updateAnimationsList; //Used to store animations in the game that we need to keep track of and update. Probably don't touch this.

class ObjectLoadInfo{
	public:
		Decal*spr;
		bool hasanim=false;
		Animation*anim;
		vi2d spos;
		vi2d size;
		Pixel col;
		bool hascut=false;
		ObjectLoadInfo(Decal*spr,Pixel col=WHITE) {
			this->spr=spr;
			this->col=col;
		}
		ObjectLoadInfo(Decal*spr,Animation*anim,Pixel col=WHITE) {
			this->spr=spr;
			this->anim=anim;
			this->hasanim=true;
			bool matches=false;
			for (int i=0;i<updateAnimationsList.size();i++) {
				if (updateAnimationsList[i]==anim) {
					matches=true;
					break;
				}
			}
			if (!matches) {
				updateAnimationsList.push_back(anim);
			}
			this->col=col;
		}
		ObjectLoadInfo(Decal*spr,vi2d spos,vi2d size,Pixel col=WHITE) {
			this->spr=spr;
			this->hascut=true;
			this->spos=spos;
			this->size=size;
			this->col=col;
		}
};

class Object{
	public:
		float x,y;
		Decal*spr;
		std::string name;
		bool hasAnim=false;
		Animation*anim;
		vi2d spos;
		vi2d size;
		bool flipped=false;
		bool hascut=false;
		bool tempObj=false;
		Pixel col;
		Object(){};
		Object(Decal*spr,Pixel col=WHITE) {
			this->spr=spr;
			this->col=col;
		}
		Object(Decal*spr,Animation*anim,Pixel col=WHITE) {
			this->spr=spr;
			this->anim=anim;
			this->hasAnim=true;
			bool matches=false;
			for (int i=0;i<updateAnimationsList.size();i++) {
				if (updateAnimationsList[i]==anim) {
					matches=true;
					break;
				}
			}
			if (!matches) {
				updateAnimationsList.push_back(anim);
			}
			this->col=col;
		}
		Object(Decal*spr,vi2d spos,vi2d size,Pixel col=WHITE) {
			this->spr=spr;
			this->hascut=true;
			this->spos=spos;
			this->size=size;
			this->col=col;
		}
};

class Map{
	public:
		std::string filename;
		Map(std::string filepath) {
			this->filename=filepath;
		}
};

class ParticleEffect{
	public:
		vf2d effectPos;
		vf2d effectSize;
		vf2d pos_low;
		vf2d pos_high;
		vf2d size_low;
		vf2d size_high;
		vf2d spd_low;
		vf2d spd_high;
		olc::Pixel col_low;
		olc::Pixel col_high;
		int amt;
		olc::Pixel foregroundColor;
		ParticleEffect(vf2d effectPos,vf2d effectSize,vf2d pos_low,vf2d pos_high,vf2d size_low,vf2d size_high,vf2d spd_low,vf2d spd_high,olc::Pixel col_low,olc::Pixel col_high,int amt,olc::Pixel foregroundColor) {
			this->effectPos=effectPos;
			this->effectSize=effectSize;
			this->pos_low=pos_low;
			this->pos_high=pos_high;
			this->size_low=size_high;
			this->spd_low=spd_low;
			this->spd_high=spd_high;
			this->col_low=col_low;
			this->col_high=col_high;
			this->amt=amt;
			this->foregroundColor=foregroundColor;
		}
};

class WEATHER_POWER{
	public:
		std::string description;
		std::string name;
		Animation*anim;
		Animation*effectAnim;
		int damage;
		int damageRoll;
		int range;
		int playerOwnCount=0;
		Pixel bgcol;
		Pixel textcol;
		int effectTime=0;
		ParticleEffect*effect;
		int seedProduction=0; //Number of seeds to add to field.
		float seedScatter=1; //Multiplier of current seeds on field.
		bool growSeeds=false; //If set to true, will trigger a growth of all seeds.
		bool burnTrees=false; //If set to true, will trigger a burning of trees.
		float treeBurnChance=1; //% chance of trees getting burned by this attack.
		float treeSeedChance=0; //% chance of trees producing seeds from this attack.
		bool lowPriority=false; //Will always go last if set to true.
		bool appliesSlow=false; //If true, will apply slow to enemies hit.
		bool appliesSpeed=false; //If true, will apply speed on use.
		bool appliesHide=false; //If true, will apply hiding on use.
		bool appliesPetrification=false; //Applies three turns of petrification on use.
		bool appliesRadioactive=false; //Radioactive removes all trees and seeds.
		float pctDmg=0; //
		bool dealsPctDmg=false; //If set to true, uses pctDmg and deals a % health value instead.
		sound::Wave*sound;
		float soundSpd=1;
		WEATHER_POWER(std::string name,std::string desc,Animation*icon,Animation*effect,int dmg,int dmgRoll,int range,Pixel bgcol,Pixel textcol,int effectTime,ParticleEffect*parteff,sound::Wave*sound,float soundSpd=1) {
			this->description=desc;
			this->name=name;
			this->anim=icon;
			this->effectAnim=effect;
			this->damage=dmg;
			this->damageRoll=dmgRoll;
			this->range=range;
			this->bgcol=bgcol;
			this->textcol=textcol;
			this->effectTime=effectTime;
			this->effect=parteff;
			this->sound=sound;
			this->soundSpd=soundSpd;
		}
};

class Entity{
	public:
		bool ally=false;
		int hp;
		int startingHP;
		Decal*startingspr;
		int maxhp;
		Decal*spr;
		float x;
		float y;
		std::string name;
		WEATHER_POWER*selectedMove;
		std::vector<WEATHER_POWER*> moveSet;
		bool turnComplete=false;
		char speed=0; //Slowed entities have a low priority. -1=Slowed, 1=Fast (high Prio), 0=Normal Priority
		int damageFrame=0;
		int fixedTurnOrderInd=0;
		bool fixedTurnOrder=false; //If this is turned on, the selected move will increment in order of the move set (and loop accordingly.)
		vf2d sprScale;
		int lastSlowVal=0; //Keeps track of last slow val. Don't take away a buff if it's different, as it was just applied.
		bool lastHiddenVal=false; //Keeps track of last hidden val. Don't take away a buff if it's different, as it was just applied.
		bool hidden=false; //If hidden, this enemy will take 0 damage.
		int shield=0; //If shield is greater than 0, the shield must be depleted first before health can be dealt.
		Entity(Decal*spr,std::string name,float x,float y,int hp,int maxhp,std::vector<WEATHER_POWER*>moveset,vf2d sprScale={1,1},bool fixedMoveset=false) {
			this->spr=this->startingspr=spr;
			this->name=name;
			this->x=x;
			this->y=y;
			this->maxhp=hp;
			this->hp=this->startingHP=hp;
			this->maxhp=maxhp;
			this->moveSet=moveset;
			this->fixedTurnOrder=fixedMoveset;
			this->sprScale=sprScale;
		}
};

class Encounter{
	public:
		int x,y;
		float playerX,playerY;
		std::vector<Entity*> entities;
		std::vector<int> turnOrder;
		Map*map;
};

class Zone{
	public:
		vi2d pos; //In tile coordinates.
		vi2d size; //In tile coordinates.
		ParticleEffect*eff;
		Zone(vi2d pos,vi2d size,vf2d effectPos,vf2d effectSize,vf2d pos_low,vf2d pos_high,vf2d size_low,vf2d size_high,vf2d spd_low,vf2d spd_high,olc::Pixel col_low,olc::Pixel col_high,int amt,olc::Pixel foregroundColor) {
			this->pos=pos;
			this->size=size;
			this->eff=new ParticleEffect(effectPos,effectSize,pos_low,pos_high,size_low,size_high,spd_low,spd_high,col_low,col_high,amt,foregroundColor);
		}
};

namespace effect {
	class Pixel{
		public:
			vf2d pos, size, spd;
			int r,g,b,a;
			int o_r,o_g,o_b,o_a;
			Pixel() {
				pos={0,0};
				size={0,0};
				spd={0,0};
			};
			Pixel(vf2d pos,vf2d size,vf2d spd,olc::Pixel col) {
				this->pos=pos;
				this->size=size;
				this->spd=spd;
				this->r=this->o_r=col.r;
				this->g=this->o_g=col.g;
				this->b=this->o_b=col.b;
				this->a=this->o_a=col.a;
			}
	};
};

class PlayerState{
	public:
		int playerHP,playerMaxHP;
		int foodCount;
		std::vector<int> weatherPowerCounts;
		float x,y; 
		bool gameFlags[128]={};
};

class SeasonsOfLoneliness : public PixelGameEngine
{
public:
	SeasonsOfLoneliness()
	{
		sAppName = "Seasons of Loneliness";
	}


public:
	sound::WaveEngine engine;
	GAMESTATE GAME_STATE=STARTING_STATE;
	int textInd=0;
	int cursorX=0;
	int transitionTime=0;
	int TIMER=0;
	bool fade=false;
	int transparency=0;
	int frameCount=0;
	float elapsedTime=0;
	const float TARGET_RATE = 1/60.0;
	std::string MAP_NAME = "";
	std::string CUTSCENE_CONSOLE_TEXT = "";
	bool GAME_FLAGS[128]={};
	int**MAP=NULL;
	int MAP_WIDTH=-1;
	int MAP_HEIGHT=-1;
	Decal*TILES;
	float PLAYER_COORDS[2] = {PLAYER_X,PLAYER_Y};
	std::vector<Object*> OBJECTS;
	bool CUTSCENE_ACTIVE=false;
	cutscene::CUTSCENE CURRENT_CUTSCENE=cutscene::NONE;
	int CUTSCENE_TIMER=0;
	bool CUTSCENE_FLAGS[8];
	Object*CUTSCENE_OBJS[8];
	int CUTSCENE_OBJ_INDICES[8];
	bool messageBoxVisible=false;
	int messageBoxCursor;
	std::string messageBoxSpeaker;
	std::string messageBoxText;
	std::string messageBoxRefText;
	bool firstNewline=false;
	bool secondNewline=false;
	bool foodMeterVisible=false;
	int foodCount=3;
	bool oxygenMeterVisible=false;
	int oxygenQualityLevel=34;
	int plantState=0b01001010010100010101010010010101;
	SplashScreen splash;
	Animation*current_playerAnim;
	Animation*playerAnim=new Animation();
	Animation*playerAnimRight=new Animation();
	Animation*playerAnimLeft=new Animation();
	Animation*playerAnimDown=new Animation();
	Animation*playerAnimWalkUp=new Animation();
	Animation*playerAnimWalkDown=new Animation();
	Animation*playerAnimWalkRight=new Animation();
	Animation*playerAnimWalkLeft=new Animation();
	Animation*playerAnimPlantUp=new Animation();
	Animation*playerAnimPlantDown=new Animation();
	Animation*playerAnimPlantRight=new Animation();
	Animation*playerAnimPlantLeft=new Animation();
	Animation*POWER_HAILSTORM_ANIMATION=new Animation();
	Animation*POWER_HURRICANE_ANIMATION=new Animation();
	Animation*POWER_METEOR_SHOWER_ANIMATION=new Animation();
	Animation*POWER_METEOR_STORM_ANIMATION=new Animation();
	Animation*POWER_SNOWSTORM_ANIMATION=new Animation();
	Animation*CONSUME_SNACK_ANIMATION=new Animation();
	Animation*CONSUME_MEAL_ANIMATION=new Animation();
	Animation*NADO_ANIMATION=new Animation();
	Animation*PETAL_STORM_ANIMATION=new Animation();
	Animation*SLEEP_ANIMATION=new Animation();
	Animation*EMPTY_BED_ANIMATION=new Animation();
	Animation*POWER_FLASHFLOOD_ANIMATION=new Animation();
	Animation*POWER_SUNNYDAY_ANIMATION=new Animation();
	Animation*POWER_FIRESTORM_ANIMATION=new Animation();
	Animation*POWER_SOLARFLARE_ANIMATION=new Animation();
	Animation*POWER_ENERGYBALL_ANIMATION=new Animation();
	ParticleEffect*HAILSTORM_EFF = new ParticleEffect({0,0},{WIDTH,HEIGHT},{0,0},{WIDTH,HEIGHT},{1,1},{4,4},{-2,0.5},{-0.5,2},Pixel(143, 242, 255,255),Pixel(255,255,255,255),300,Pixel(220, 226, 227,0));
	ParticleEffect*HURRICANE_EFF = new ParticleEffect({0,0},{WIDTH,HEIGHT},{0,0},{WIDTH,HEIGHT},{1,1},{2,2},{-4,1},{-1,4},Pixel(225, 248, 252,64),Pixel(255,255,255,128),300,Pixel(220, 226, 227,0));
	ParticleEffect*METEOR_RAIN_EFF = new ParticleEffect({0,0},{WIDTH,HEIGHT},{0,0},{WIDTH,HEIGHT},{3,3},{6,6},{-1,0.2},{-0.2,1},Pixel(46, 31, 31,255),Pixel(43, 31, 46,255),50,Pixel(30, 10, 36,0));
	ParticleEffect*METEOR_STORM_EFF = new ParticleEffect({0,0},{WIDTH,HEIGHT},{0,0},{WIDTH,HEIGHT},{3,3},{10,10},{-2,0.1},{-0.1,2},Pixel(46, 31, 31,255),Pixel(181, 72, 46,255),75,Pixel(30, 10, 36,48));
	ParticleEffect*SNOWSTORM_EFF = new ParticleEffect({0,0},{WIDTH,HEIGHT},{0,0},{WIDTH,HEIGHT},{1,1},{3,3},{-1,0.5},{1,2},Pixel(193, 198, 201,200),Pixel(255,255,255,255),400,Pixel(220, 226, 227,0));
	ParticleEffect*SANDSTORM_EFF = new ParticleEffect({0,0},{WIDTH,HEIGHT},{0,0},{WIDTH,HEIGHT},{1,1},{3,3},{-20,1},{20,3},Pixel(133, 98, 66,130),Pixel(158, 93, 33,210),300,Pixel(87, 78, 69,0));
	ParticleEffect*SEED_STORM_EFF = new ParticleEffect({0,0},{WIDTH,HEIGHT},{0,0},{WIDTH,HEIGHT},{1,1},{2,2},{-0.5,-1},{0.5,-0.2},Pixel(65, 98, 66,130),Pixel(65, 117, 60,210),100,Pixel(70, 158, 62,0));
	ParticleEffect*AVALANCHE_EFF = new ParticleEffect({0,0},{WIDTH,HEIGHT},{0,0},{WIDTH,HEIGHT},{1,1},{8,8},{-4,2},{4,4},Pixel(143, 242, 255,255),Pixel(255,255,255,255),300,Pixel(220, 226, 227,0));
	ParticleEffect*CONSUME_SNACK_EFF = new ParticleEffect({0,0},{64,64},{64,64},{64,64},{1,1},{2,2},{-0.1,-0.5},{0.1,-0.05},Pixel(255,255,255,130),Pixel(255,255,255,210),30,Pixel(147, 161, 90,0)); //Used for CONSUME_MEAL too.
	ParticleEffect*PETAL_STORM_EFF = new ParticleEffect({0,0},{WIDTH,HEIGHT},{0,0},{WIDTH,HEIGHT},{1,1},{3,3},{-4,0.6},{4,2},Pixel(127, 27, 130,130),Pixel(227, 218, 227,240),160,Pixel(147, 161, 90,0));
	ParticleEffect*LIGHT_STORM_EFF = new ParticleEffect({0,0},{WIDTH,HEIGHT},{0,0},{WIDTH,HEIGHT},{1,HEIGHT/2},{2,HEIGHT},{0,4},{0,25},Pixel(214, 233, 255,130),Pixel(255, 255, 255,255),30,Pixel(0, 0, 0,32));
	ParticleEffect*SEED_BULLET_EFF = new ParticleEffect({0,0},{64,64},{0,0},{WIDTH,HEIGHT},{1,1},{3,3},{-5,-5},{5,5},Pixel(138, 255, 153,200),Pixel(193, 245, 200,210),40,Pixel(70, 158, 62,0));
	ParticleEffect*SEED_PELLET_EFF = new ParticleEffect({0,0},{64,64},{0,0},{WIDTH,HEIGHT},{1,1},{1,1},{-3,-3},{3,3},Pixel(138, 255, 153,200),Pixel(193, 245, 200,210),10,Pixel(70, 158, 62,0));
	ParticleEffect*SUMMON_MINION_EFF = new ParticleEffect({0,0},{WIDTH,HEIGHT},{0,0},{WIDTH,HEIGHT},{4,4},{12,12},{0,-6},{0,-20},Pixel(223, 242, 0,130),Pixel(223, 255, 114,210),50,Pixel(70, 158, 62,0));
	ParticleEffect*SEED_OF_LIFE_EFF = new ParticleEffect({0,0},{WIDTH,HEIGHT},{0,0},{WIDTH,HEIGHT},{2,2},{2,2},{-1,-2},{1,-0.2},Pixel(65, 98, 66,210),Pixel(65, 117, 60,255),140,Pixel(70, 158, 62,0));
	ParticleEffect*FIRESTORM_EFF = new ParticleEffect({0,0},{WIDTH,HEIGHT},{0,0},{WIDTH,HEIGHT},{4,4},{12,12},{-12,-8},{12,-4},Pixel(209, 90, 69,130),Pixel(209, 141, 69,255),140,Pixel(255, 0, 0,32));
	ParticleEffect*ACID_RAIN_EFF = new ParticleEffect({0,0},{WIDTH,HEIGHT},{0,0},{WIDTH,HEIGHT},{1,1},{2,2},{-4,1},{-1,4},Pixel(117, 125, 57,128),Pixel(224, 245, 66,210),300,Pixel(220, 226, 227,0));
	ParticleEffect*DROUGHT_EFF = new ParticleEffect({0,0},{WIDTH,HEIGHT},{0,0},{WIDTH,HEIGHT},{1,1},{2,2},{-1,-4},{1,-0.5},Pixel(217, 79, 33,128),Pixel(217, 162, 33,210),70,Pixel(217, 204, 173,64));
	ParticleEffect*HEAT_WAVE_EFF = new ParticleEffect({0,0},{WIDTH,HEIGHT},{0,0},{WIDTH,HEIGHT},{4,4},{12,12},{-8,4},{-4,8},Pixel(209, 90, 69,130),Pixel(209, 141, 69,255),80,Pixel(255, 0, 0,0));
	ParticleEffect*TORNADO_EFF = new ParticleEffect({0,0},{64,64},{0,0},{64,64},{1,2},{3,20},{-14,2},{14,12},Pixel(168, 155, 146,196),Pixel(196, 194, 192,220),45,Pixel(255, 0, 0,0));
	ParticleEffect*GUST_EFF = new ParticleEffect({0,0},{64,64},{0,0},{64,64},{1,2},{3,20},{-6,1},{6,6},Pixel(168, 155, 146,196),Pixel(196, 194, 192,220),20,Pixel(255, 0, 0,0));
	ParticleEffect*FLASH_FLOOD_EFF = new ParticleEffect({0,0},{WIDTH,HEIGHT},{0,0},{WIDTH,HEIGHT},{1,1},{2,2},{-4,1},{-1,4},Pixel(8, 54, 204,64),Pixel(255,255,255,128),500,Pixel(220, 226, 227,0));
	ParticleEffect*SUNNYDAY_EFF = new ParticleEffect({0,0},{WIDTH,HEIGHT},{0,0},{WIDTH,HEIGHT},{1,1},{4,4},{0,-2},{0,-6},Pixel(209, 207, 113,130),Pixel(255, 255, 255,210),50,Pixel(70, 158, 62,0));
	ParticleEffect*SOLARFLARE_EFF = new ParticleEffect({0,0},{128,128},{0,0},{128,128},{2,HEIGHT/2},{4,HEIGHT},{0,4},{0,25},Pixel(217, 36, 0,130),Pixel(217, 94, 0,255),40,Pixel(166, 51, 28,64));
	ParticleEffect*HIDE_EFF = new ParticleEffect({0,0},{64,64},{0,0},{64,64},{32,32},{32,32},{-1,-1},{1,1},Pixel(0, 0, 0,25),Pixel(0, 0, 0,75),5,Pixel(166, 51, 28,0));
	ParticleEffect*HYPERZAP_EFF = new ParticleEffect({0,0},{64,64},{0,0},{64,64},{1,1},{6,6},{0,-2},{0,-10},Pixel(232, 2, 230,130),Pixel(250, 81, 247,210),30,Pixel(70, 158, 62,0));
	ParticleEffect*POLLINATION_EFF = new ParticleEffect({0,0},{WIDTH,HEIGHT},{0,0},{WIDTH,HEIGHT},{4,4},{12,12},{-20,-6},{0,-20},Pixel(130, 245, 2,130),Pixel(250, 246, 2,210),50,Pixel(199, 199, 52,64));
	ParticleEffect*GLARE_EFF = new ParticleEffect({0,0},{WIDTH,HEIGHT},{0,0},{WIDTH,HEIGHT},{4,4},{12,12},{-20,-6},{0,-20},Pixel(130, 245, 2,130),Pixel(250, 246, 2,210),0,Pixel(255, 255, 255,64));
	ParticleEffect*PETRIFY_EFF = new ParticleEffect({0,0},{64,64},{0,0},{64,64},{2,2},{4,4},{-1,-3},{1,-8},Pixel(166, 147, 143,200),Pixel(199, 194, 193,210),30,Pixel(199, 199, 52,0));
	ParticleEffect*MEGAFANG_EFF = new ParticleEffect({0,0},{64,64},{0,0},{64,64},{1,1},{3,3},{-5,-5},{5,5},Pixel(255,255,255,200),Pixel(255,255,255,210),40,Pixel(70, 158, 62,0));
	ParticleEffect*END_OF_THE_CENTURY_BEAM_EFF = new ParticleEffect({0,0},{WIDTH,HEIGHT},{WIDTH,HEIGHT},{64,64},{8,8},{20,20},{-4,10},{4,35},Pixel(0,0,0,75),Pixel(255,255,255,210),120,Pixel(255, 255, 255,96));
	ParticleEffect*RADIOACTIVE_TRANSMISSION_EFF = new ParticleEffect({0,0},{WIDTH,HEIGHT},{WIDTH,HEIGHT},{64,64},{5,5},{10,10},{0,0},{0,0},Pixel(70, 189, 23,196),Pixel(189, 189, 23,210),250,Pixel(0,255,0,128));
	ParticleEffect*ENERGY_BALL_EFF = new ParticleEffect({0,0},{64,64},{0,0},{64,64},{5,5},{10,10},{0,0},{0,0},Pixel(70, 189, 23,75),Pixel(189, 189, 23,150),100,Pixel(0,255,0,0));
	WEATHER_POWER*HAILSTORM = new WEATHER_POWER("Hailstorm","Causes a flurry of hard cold rocks to be unleashed in target area. 60+1d30",POWER_HAILSTORM_ANIMATION,POWER_HAILSTORM_ANIMATION,60,30,160,Pixel(72, 160, 212,255),Pixel(93, 161, 163,255),120,HAILSTORM_EFF,&SOUND_WEATHERLIGHT);
	WEATHER_POWER*HURRICANE = new WEATHER_POWER("Hurricane","Causes heavy winds, scattering seeds, heavy rain. 20+1d10",POWER_HURRICANE_ANIMATION,POWER_HURRICANE_ANIMATION,20,10,200,Pixel(99, 148, 132,255),Pixel(121, 132, 140,255),120,HURRICANE_EFF,&SOUND_WEATHERHEAVY);
	WEATHER_POWER*METEOR_RAIN = new WEATHER_POWER("Meteor Rain","Causes fiery space rocks to fall on target area. 50+1d50",POWER_METEOR_SHOWER_ANIMATION,POWER_METEOR_SHOWER_ANIMATION,50,50,96,Pixel(96, 86, 153,255),Pixel(170, 103, 201,255),120,METEOR_STORM_EFF,&SOUND_WEATHERHEAVY);
	WEATHER_POWER*METEOR_STORM = new WEATHER_POWER("Meteor Storm","Larger burning space rocks. 120+60d",POWER_METEOR_STORM_ANIMATION,POWER_METEOR_STORM_ANIMATION,120,60,140,Pixel(89, 4, 33,255),Pixel(130, 56, 1,255),120,METEOR_RAIN_EFF,&SOUND_WEATHERHEAVY);
	WEATHER_POWER*SNOWSTORM = new WEATHER_POWER("Snowstorm","Slows down targets and causes rapid temperature drops. 15+1d10",POWER_SNOWSTORM_ANIMATION,POWER_SNOWSTORM_ANIMATION,15,10,140,Pixel(183, 196, 194,255),Pixel(222, 255, 254,255),120,SNOWSTORM_EFF,&SOUND_WEATHERHEAVY);
	WEATHER_POWER*SANDSTORM = new WEATHER_POWER("Sandstorm","",POWER_HAILSTORM_ANIMATION,POWER_HAILSTORM_ANIMATION,5,10,64,Pixel(93, 161, 163,255),Pixel(72, 160, 212,255),120,SANDSTORM_EFF,&SOUND_WEATHERLIGHT,0.6);
	WEATHER_POWER*SEED_STORM = new WEATHER_POWER("Seed Storm","",POWER_HAILSTORM_ANIMATION,POWER_HAILSTORM_ANIMATION,-10,15,30,Pixel(93, 161, 163,255),Pixel(72, 160, 212,255),120,SEED_STORM_EFF,&SOUND_LASERSHOOT,0.4);
	WEATHER_POWER*AVALANCHE = new WEATHER_POWER("Avalanche","",POWER_HAILSTORM_ANIMATION,POWER_HAILSTORM_ANIMATION,15,10,200,Pixel(93, 161, 163,255),Pixel(72, 160, 212,255),120,AVALANCHE_EFF,&SOUND_WEATHERHEAVY);
	WEATHER_POWER*LIGHT_STORM = new WEATHER_POWER("Light Storm","",POWER_HAILSTORM_ANIMATION,POWER_HAILSTORM_ANIMATION,50,9,64,Pixel(171, 151, 53,255),Pixel(237, 237, 237,255),120,LIGHT_STORM_EFF,&SOUND_SAW,0.7);
	WEATHER_POWER*SEED_BULLET = new WEATHER_POWER("Seed Bullet","",POWER_HAILSTORM_ANIMATION,POWER_HAILSTORM_ANIMATION,10,15,30,Pixel(57, 92, 63,255),Pixel(95, 232, 119,255),120,SEED_BULLET_EFF,&SOUND_LASERSHOOT,0.8);
	WEATHER_POWER*SEED_PELLET = new WEATHER_POWER("Seed Pellet","",POWER_HAILSTORM_ANIMATION,POWER_HAILSTORM_ANIMATION,5,10,30,Pixel(57, 92, 63,255),Pixel(95, 232, 119,255),120,SEED_PELLET_EFF,&SOUND_MSG,0.7);
	WEATHER_POWER*SEED_OF_LIFE = new WEATHER_POWER("Seed of Life","",POWER_HAILSTORM_ANIMATION,POWER_HAILSTORM_ANIMATION,-10,15,30,Pixel(93, 161, 163,255),Pixel(72, 160, 212,255),120,SEED_OF_LIFE_EFF,&SOUND_WEATHERLIGHT);
	WEATHER_POWER*SUMMON_MINIONS = new WEATHER_POWER("Summon Minions","",POWER_HAILSTORM_ANIMATION,POWER_HAILSTORM_ANIMATION,-75,15,255,Pixel(57, 92, 63,255),Pixel(95, 232, 119,255),120,SUMMON_MINION_EFF,&SOUND_WEATHERLIGHT);
	WEATHER_POWER*CONSUME_SNACK = new WEATHER_POWER("Snack","Restores 33% health for 5 turns. If battle ends before effect ends, food is not consumed.",CONSUME_SNACK_ANIMATION,CONSUME_SNACK_ANIMATION,-1001,1,200,Pixel(147, 173, 66,255),Pixel(255, 188, 3,255),120,CONSUME_SNACK_EFF,&SOUND_SELECT,0.7);
	WEATHER_POWER*CONSUME_MEAL = new WEATHER_POWER("Meal","Restores all health. Increases Maximum Health by 5.",CONSUME_MEAL_ANIMATION,CONSUME_MEAL_ANIMATION,-1002,1,200,Pixel(147, 173, 66,255),Pixel(255, 188, 3,255),120,CONSUME_SNACK_EFF,&SOUND_SELECT,0.7);
	WEATHER_POWER*PETAL_STORM = new WEATHER_POWER("Petal Storm","Places seeds, causes minor healing. 20+1d20",PETAL_STORM_ANIMATION,PETAL_STORM_ANIMATION,20,20,110,Pixel(189, 132, 189,255),Pixel(235, 75, 235,255),120,PETAL_STORM_EFF,&SOUND_WEATHERHEAVY);
	WEATHER_POWER*ACID_RAIN = new WEATHER_POWER("Acid Rain","",POWER_HURRICANE_ANIMATION,POWER_HURRICANE_ANIMATION,20,10,150,Pixel(99, 148, 132,255),Pixel(121, 132, 140,255),120,ACID_RAIN_EFF,&SOUND_WEATHERHEAVY);
	WEATHER_POWER*DROUGHT = new WEATHER_POWER("Drought","Halves healing effects for the next turn.",POWER_HURRICANE_ANIMATION,POWER_HURRICANE_ANIMATION,8,3,150,Pixel(99, 148, 132,255),Pixel(121, 132, 140,255),120,DROUGHT_EFF,&SOUND_CHANGE_OPTION,0.6);
	WEATHER_POWER*HEAT_WAVE = new WEATHER_POWER("Heat Wave","",POWER_HURRICANE_ANIMATION,POWER_HURRICANE_ANIMATION,20,12,150,Pixel(99, 148, 132,255),Pixel(121, 132, 140,255),120,HEAT_WAVE_EFF,&SOUND_WEATHERLIGHT);
	WEATHER_POWER*TORNADO = new WEATHER_POWER("Tornado","",POWER_HURRICANE_ANIMATION,POWER_HURRICANE_ANIMATION,30,10,150,Pixel(99, 148, 132,255),Pixel(121, 132, 140,255),120,TORNADO_EFF,&SOUND_WEATHERHEAVY,0.9);
	WEATHER_POWER*GUST = new WEATHER_POWER("Gust","",POWER_HURRICANE_ANIMATION,POWER_HURRICANE_ANIMATION,15,10,150,Pixel(99, 148, 132,255),Pixel(121, 132, 140,255),120,GUST_EFF,&SOUND_WEATHERLIGHT,0.9);
	WEATHER_POWER*FLASH_FLOOD = new WEATHER_POWER("Flash Flood","Causes massive seed growth and heavy damage. 30+1d20",POWER_FLASHFLOOD_ANIMATION,POWER_FLASHFLOOD_ANIMATION,30,20,200,Pixel(27, 41, 87,255),Pixel(138, 161, 237,255),120,FLASH_FLOOD_EFF,&SOUND_WEATHERHEAVY);
	WEATHER_POWER*SUNNY_DAY = new WEATHER_POWER("Sunny Day","Fertilize soil, multiplying seed growth and causing trees to drop seeds. 10+1d10",POWER_SUNNYDAY_ANIMATION,POWER_SUNNYDAY_ANIMATION,10,10,200,Pixel(179, 164, 71,255),Pixel(222, 198, 44,255),120,SUNNYDAY_EFF,&SOUND_CHANGE_OPTION,0.9);
	WEATHER_POWER*FIRESTORM = new WEATHER_POWER("Fire Storm","Cause devastating fires, destroying everything in sight. 65+1d40",POWER_FIRESTORM_ANIMATION,POWER_FIRESTORM_ANIMATION,65,40,145,Pixel(176, 95, 44,255),Pixel(237, 100, 14,255),120,FIRESTORM_EFF,&SOUND_EXPLODE,0.9);
	WEATHER_POWER*SOLAR_FLARE = new WEATHER_POWER("Solar Flare","A concentrated sunbeam of death. Burns down all trees. 175+1d40",POWER_SOLARFLARE_ANIMATION,POWER_SOLARFLARE_ANIMATION,175,40,32,Pixel(176, 53, 37,255),Pixel(217, 98, 0,255),120,SOLARFLARE_EFF,&SOUND_LASERSHOOT);
	WEATHER_POWER*HIDE = new WEATHER_POWER("Hide","",POWER_SOLARFLARE_ANIMATION,POWER_SOLARFLARE_ANIMATION,175,40,32,Pixel(176, 53, 37,255),Pixel(217, 98, 0,255),120,HIDE_EFF,&SOUND_WEATHERLIGHT,0.7);
	WEATHER_POWER*HYPERZAP = new WEATHER_POWER("Hyper Zap","",POWER_SOLARFLARE_ANIMATION,POWER_SOLARFLARE_ANIMATION,175,40,32,Pixel(176, 53, 37,255),Pixel(217, 98, 0,255),120,HYPERZAP_EFF,&SOUND_LASERSHOOT);
	WEATHER_POWER*POLLINATION = new WEATHER_POWER("Pollination","",POWER_SOLARFLARE_ANIMATION,POWER_SOLARFLARE_ANIMATION,-110,10,255,Pixel(176, 53, 37,255),Pixel(217, 98, 0,255),120,POLLINATION_EFF,&SOUND_WEATHERLIGHT);
	WEATHER_POWER*GLARE = new WEATHER_POWER("Glare","",POWER_SOLARFLARE_ANIMATION,POWER_SOLARFLARE_ANIMATION,1,10,255,Pixel(176, 53, 37,255),Pixel(217, 98, 0,255),120,GLARE_EFF,&SOUND_MSG,0.8);
	WEATHER_POWER*PETRIFY = new WEATHER_POWER("Petrify","",POWER_SOLARFLARE_ANIMATION,POWER_SOLARFLARE_ANIMATION,1,10,255,Pixel(176, 53, 37,255),Pixel(217, 98, 0,255),120,PETRIFY_EFF,&SOUND_EXPLODE,0.6);
	WEATHER_POWER*MEGA_FANG = new WEATHER_POWER("Mega Fang","",POWER_SOLARFLARE_ANIMATION,POWER_SOLARFLARE_ANIMATION,1,10,255,Pixel(176, 53, 37,255),Pixel(217, 98, 0,255),120,MEGAFANG_EFF,&SOUND_EXPLODE);
	WEATHER_POWER*END_OF_THE_CENTURY_BEAM = new WEATHER_POWER("End of the Century Beam","",POWER_SOLARFLARE_ANIMATION,POWER_SOLARFLARE_ANIMATION,1,10,255,Pixel(176, 53, 37,255),Pixel(217, 98, 0,255),160,END_OF_THE_CENTURY_BEAM_EFF,&SOUND_EXPLODE);
	WEATHER_POWER*RADIOACTIVE_TRANSMISSION = new WEATHER_POWER("Radioactive Transmission","",POWER_SOLARFLARE_ANIMATION,POWER_SOLARFLARE_ANIMATION,1,10,255,Pixel(176, 53, 37,255),Pixel(217, 98, 0,255),160,RADIOACTIVE_TRANSMISSION_EFF,&SOUND_SONAR);
	WEATHER_POWER*ENERGY_BALL = new WEATHER_POWER("Energy Ball","Your last resort. A jolt of power. 20+1d10",POWER_ENERGYBALL_ANIMATION,POWER_ENERGYBALL_ANIMATION,20,10,64,Pixel(102, 156, 81,255),Pixel(26, 120, 42,255),120,RADIOACTIVE_TRANSMISSION_EFF,&SOUND_MSG,0.8);

	bool IN_BATTLE_ENCOUNTER = false;
	int BATTLE_ENTRY_TIMER = 0;
	int EFFECT_TIMER = 0;
	#define WEATHER_POWER_COUNT 13 //Number of powers that are in the game. Update storage array accordingly.
	WEATHER_POWER*WEATHER_POWERS[WEATHER_POWER_COUNT] = {
		HAILSTORM,
		PETAL_STORM,
		HURRICANE,
		METEOR_RAIN,
		METEOR_STORM,
		SNOWSTORM,
		FLASH_FLOOD,
		SUNNY_DAY,
		FIRESTORM,
		SOLAR_FLARE,
		CONSUME_SNACK,
		CONSUME_MEAL,
		ENERGY_BALL};
	battle::BATTLESTATE BATTLE_STATE=battle::NONE;
	std::vector<WEATHER_POWER*> availablePowers;
	WEATHER_POWER*BATTLE_CARD_SELECTION=HAILSTORM;
	int BATTLE_CARD_SELECTION_IND=0;
	int PLAYER_SELECTED_TARGET=-1;
	int PLAYER_MAXHP=60;
	int PLAYER_HP=PLAYER_MAXHP;
	int BATTLE_CURRENT_TURN_ENTITY=-1;
	int CURRENT_ENCOUNTER_IND=-1;
	unsigned int RAND_CALLS=0;
	std::vector<DisplayNumber*> BATTLE_DISPLAY_NUMBERS;
	bool PLAYER_TURN_COMPLETE=false;
	int FOOD_REGEN_TURNS=0;
	WEATHER_POWER* CUTSCENE_DISPLAYED_CARD;
	Pixel FOREGROUND_EFFECT_COLOR;
	Pixel ORIGINAL_FOREGROUND_EFFECT_COLOR;
	int PIXEL_LIMIT=0;
	vf2d PIXEL_POS={0,0};
	vf2d PIXEL_SIZE={WIDTH,HEIGHT};
	vf2d STAR_POS={0,0};
	vf2d STAR_SIZE={WIDTH,HEIGHT};
	float PIXEL_EFFECT_TRANSPARENCY=0;
	int CUTSCENE_OBJ_COUNT=0;
	int TERMINAL_SELECTED_CHAR=0;
	std::string TERMINAL_INPUT="";
	std::string PLAYER_NAME="...";
	std::vector<Zone*>ZONES;
	std::vector<vi2d>COLLECTED_ITEMS;
	Zone*ACTIVE_ZONE=nullptr;
	std::vector<vi2d>REPAIRED_ROVERS;
	std::string CONSOLE_REF_TEXT;
	bool SOUND_IS_PLAYING=false;
	int MAIN_MENU_SELECTION=0;
	std::string MENU_OPTIONS[4]={"NEW GAME","MUTE SOUND","EXIT"};
	int BATTLE_REASON_CODE=-1;
	bool BATTLE_DROUGHT_ACTIVE=false; 
	bool ROCKET_BOARD_OPTION=false;
	int LOCKED_IN_DELAY=0;
	Pixel SHIP_COLOR=WHITE;
	vf2d BOSS_SIZE={1,1};
	vf2d CREDITS_SCROLLING_OFFSET={0,12};
	bool HIDE_CARDS=false;
	bool USE_TOUCH_CONTROLS=false;
	bool MOUSE_DOWN=false;

	Map*MAP_1=new Map("map1");
	Map*MAP_2=new Map("map2");
	Map*MAP_3=new Map("map3");
	Map*MAP_4=new Map("map4");
	Map*MAP_5=new Map("map5");
	Map*MAP_6=new Map("map6");

	Decal*DOME_DECAL,*FOOD_METER_DECAL,*OXYGEN_METER_DECAL,*PLANT_DECAL,
	*PLAYER_DECAL,
	*WEATHERNODE_EFFECT_DECAL,*POWER_HAILSTORM_DECAL,*POWER_HURRICANE_DECAL,*POWER_METEOR_SHOWER_DECAL,*POWER_METEOR_STORM_DECAL,*POWER_SNOWSTORM_DECAL,
	*SPIDEY_DECAL,*TARGETING_CIRCLE,*TARGETING_RANGE_CIRCLE,*HEALTHBAR_DECAL,
	*CONSUME_SNACK_DECAL,*CONSUME_MEAL_DECAL,*COMPUTER_DECAL,*BROKEN_ROVER_DECAL,
	*NADO_DECAL,*SILICON_ROCK_DECAL,*PETAL_STORM_DECAL,*ROVER_DECAL,*X_X_DECAL,
	*LATER_THAT_NIGHT_DECAL,*SLEEP_DECAL,*SEED_DECAL,*TREE_DECAL,*X_X_UNCHARGED_DECAL,
	*SANDWORM_DECAL,*SNAKE_DECAL,*MOTH_DECAL,*FLASH_FLOOD_DECAL,*SUNNY_DAY_DECAL,*FIRESTORM_DECAL,*SOLARFLARE_DECAL,
	*HP_REGEN_DECAL,*SLOWED_DECAL, *SPEED_DECAL,*HIDDEN_DECAL,*BOOK_DECAL,*BOOK2_DECAL,
	*LAUNCHPAD_CLOSED_DECAL,*LAUNCHPAD_HALF1_DECAL,*LAUNCHPAD_HALF2_DECAL,*LAUNCHPAD_DECAL,*Y_Y_DECAL,
	*PETRIFY_DECAL,*A_A_DECAL,*A_A_RECHARGE_DECAL, *ENERGYBALL_DECAL, *TOUCHSCREEN_DECAL;
	std::map<std::string,ObjectLoadInfo*> BASE_OBJECTS;
	std::vector<Encounter> ENCOUNTERS;
	Encounter ENCOUNTER_SPIDEY_1;
	Encounter ENCOUNTER_X_X;
	Encounter ENCOUNTER_SANDWORM_1;
	Encounter ENCOUNTER_MEGAMOTH;
	Encounter ENCOUNTER_SNAKEPACK;
	Encounter ENCOUNTER_Y_Y;
	Encounter ENCOUNTER_A_A;
	Encounter CURRENT_ENCOUNTER;
	std::vector<WEATHER_POWER*>MOVESET_SPIDEY;
	std::vector<WEATHER_POWER*>MOVESET_SANDWORM;
	std::vector<WEATHER_POWER*>MOVESET_SNAKE;
	std::vector<WEATHER_POWER*>MOVESET_MOTH;
	std::vector<WEATHER_POWER*>MOVESET_X_X;
	std::vector<WEATHER_POWER*>MOVESET_XMINION;
	std::vector<WEATHER_POWER*>MOVESET_MEGAMOTH;
	std::vector<WEATHER_POWER*>MOVESET_SIDEWINDER;
	std::vector<WEATHER_POWER*>MOVESET_Y_Y;
	std::vector<WEATHER_POWER*>MOVESET_A_A;
	std::vector<Seed*>SEEDS;
	std::vector<Seed*>TREES;
	bool SOUND_IS_MUTED=false;
	bool END_THE_GAME=false;
	bool audioFade=false;
	float audioLevel=0.6;
	int SEED_COUNT=0;
	vf2d BATTLE_PLAYER_COORDS;
	PlayerState*PREV_PLAYERSTATE=new PlayerState();
	int WALK_STEPS=0;
	int PETRIFY_TURNS=0;
	int LAST_FRAME_SOUND_PLAYED=0;
	bool queueBGMPlayback=false;
	bool MOUSE_PRESSED_DOWN=false;
	bool MOUSE_RELEASED=false;
	
	Map*CURRENT_MAP=MAP_1;


	sound::Wave SONG_MAIN,SONG_BATTLE,SONG_FINALBATTLE,SONG_EXPLORE,SONG_DOME,SONG_GAMEOVER;
	sound::Wave SOUND_MSG,SOUND_CHANGE_OPTION,SOUND_ROBOTICNOISE,SOUND_SELECT,SOUND_SAW,SOUND_SONAR,SOUND_WARNING,SOUND_EXPLODE,SOUND_LASERSHOOT,SOUND_WEATHERHEAVY,SOUND_WEATHERLIGHT,SOUND_INTROFINAL;

	std::queue<int> turnOrder;

	#define MAX_PIXELS 500
	effect::Pixel*pixels[MAX_PIXELS];
	effect::Pixel*starpixels[150];

	sound::PlayingWave CURRENT_BGM;

	bool OnUserCreate() override
	{
		SetPixelMode(Pixel::ALPHA);
		ConsoleCaptureStdOut(true);
		engine.InitialiseAudio();

		#define LAYERS 3 //How many layers exist.
		for (int i=1;i<LAYERS;i++) {
			CreateLayer();
			EnableLayer(i,true);
		}

		if (SKIP_INTRO||SKIP_CHAPTER1||SKIP_CHAPTER2||SKIP_CHAPTER3||SKIP_LAUNCHPAD) {
			GAME_FLAGS[gameflag::TUTORIAL_WALKED_OFF_FARM]=true;
			GAME_FLAGS[gameflag::VISIT_BROKEN_ROVER]=true;
			GAME_STATE=GAMEWORLD;
		}
		if (SKIP_CHAPTER1||SKIP_CHAPTER2||SKIP_CHAPTER3||SKIP_LAUNCHPAD) {
			GAME_FLAGS[gameflag::COLLECTED_SILICON_1]=true;
			GAME_FLAGS[gameflag::REPAIRED_ROVER_1]=true;
			GAME_FLAGS[gameflag::REST_IN_DOME]=true;
			GAME_STATE=GAMEWORLD;
		}
		if (SKIP_CHAPTER2||SKIP_CHAPTER3||SKIP_LAUNCHPAD) {
			//GAME_FLAGS[gameflag::CHECK_ROVER_3]=true;
			GAME_FLAGS[gameflag::COLLECTED_SILICON_3]=true;
			GAME_FLAGS[gameflag::CHECK_ROVER_2]=true;
		}
		if (SKIP_CHAPTER3||SKIP_LAUNCHPAD) {
			GAME_FLAGS[gameflag::CHECK_ROVER_4]=true;
			GAME_FLAGS[gameflag::LITTLESNAKES_KILLED]=true;
			GAME_FLAGS[gameflag::COLLECTED_SILICON_4]=true;
		}
		if (SKIP_LAUNCHPAD) {
			GAME_FLAGS[gameflag::CHECK_ROVER_5]=true;
			GAME_FLAGS[gameflag::ROCKET_LAUNCH_READY]=true;
			GAME_FLAGS[gameflag::COLLECTED_ITEMS_IN_DOME]=true;
			GAME_FLAGS[gameflag::BOARDED_ROCKET]=true;
		}
		if (SKIP_CHAPTER2||SKIP_BOSS) {
			GAME_FLAGS[gameflag::SLEEP]=true;
			GAME_FLAGS[gameflag::FIRST_ENCOUNTER_X_X]=true;
			GAME_FLAGS[gameflag::ANALYSIS_X_X]=true;
			GAME_FLAGS[gameflag::TUTORIAL1_X_X]=true;
			GAME_FLAGS[gameflag::TUTORIAL2_X_X]=true;
			GAME_STATE=GAMEWORLD;
		}
		if (SKIP_CHAPTER2||SKIP_SILICON2) {
			GAME_FLAGS[gameflag::CHECK_ROVER]=true;
			GAME_FLAGS[gameflag::NEXT_COORDS]=true;
			GAME_FLAGS[gameflag::COLLECTED_SILICON_2]=true;
			GAME_STATE=GAMEWORLD;
		}
		if (SKIP_STORYBOOK) {
			//GAME_FLAGS[gameflag::CHECK_ROVER_3]=true;
			GAME_FLAGS[gameflag::CHECK_ROVER_3]=true;
			GAME_FLAGS[gameflag::STORY_REVIEW]=true;
			GAME_FLAGS[gameflag::NEXT_COORDS2]=true;
			PLAYER_HP=PLAYER_MAXHP=100;
			PLAYER_COORDS[0]=38;
			PLAYER_COORDS[1]=38;
		}

		SONG_MAIN = sound::Wave("./assets/SeasonsOfLoneliness.wav");
		SONG_BATTLE = sound::Wave("./assets/battle.wav");
		SONG_FINALBATTLE = sound::Wave("./assets/finalBattle.wav");
		SONG_EXPLORE = sound::Wave("./assets/explore.wav");
		SONG_GAMEOVER = sound::Wave("./assets/gameover.wav");
		SONG_DOME = sound::Wave("./assets/dome.wav");
		SOUND_MSG = sound::Wave("./assets/msg.wav");
		SOUND_CHANGE_OPTION = sound::Wave("./assets/card_flip.wav");
		SOUND_ROBOTICNOISE = sound::Wave("./assets/roboticNoise.wav");
		SOUND_SELECT = sound::Wave("./assets/select.wav");
		SOUND_SAW = sound::Wave("./assets/saw.wav");
		SOUND_SONAR = sound::Wave("./assets/sonar.wav");
		SOUND_WARNING = sound::Wave("./assets/warning.wav");
		SOUND_EXPLODE = sound::Wave("./assets/explode.wav");
		SOUND_LASERSHOOT = sound::Wave("./assets/laserShoot.wav");
		SOUND_WEATHERHEAVY = sound::Wave("./assets/weatherHeavy.wav");
		SOUND_WEATHERLIGHT = sound::Wave("./assets/weatherLight.wav");
		SOUND_INTROFINAL = sound::Wave("./assets/IntroFinal.wav");

		//ConsoleShow(F1,false);
		// Called once at the start, so create things here
		TILES=new Decal(new Sprite("assets/tiles.png"));
		DOME_DECAL=new Decal(new Sprite("assets/dome.png"));
		FOOD_METER_DECAL=new Decal(new Sprite("assets/corn.png"));
		OXYGEN_METER_DECAL=new Decal(new Sprite("assets/co2.png"));
		PLANT_DECAL=new Decal(new Sprite("assets/plant.png"));
		PLAYER_DECAL=new Decal(new Sprite("assets/player.png"));
		WEATHERNODE_EFFECT_DECAL=new Decal(new Sprite("assets/weathernode_effect.png"));
		POWER_HAILSTORM_DECAL=new Decal(new Sprite("assets/hailstorm_icon.png"));
		POWER_HURRICANE_DECAL=new Decal(new Sprite("assets/hurricane_icon.png"));
		POWER_METEOR_SHOWER_DECAL=new Decal(new Sprite("assets/meteor_shower_icon.png"));
		POWER_METEOR_STORM_DECAL=new Decal(new Sprite("assets/meteor_storm.png"));
		POWER_SNOWSTORM_DECAL=new Decal(new Sprite("assets/snowstorm_icon.png"));
		CONSUME_SNACK_DECAL=new Decal(new Sprite("assets/snack.png"));
		CONSUME_MEAL_DECAL=new Decal(new Sprite("assets/meal.png"));
		SPIDEY_DECAL=new Decal(new Sprite("assets/spidey.png"));
		TARGETING_CIRCLE=new Decal(new Sprite("assets/targetCircle.png"));
		TARGETING_RANGE_CIRCLE=new Decal(new Sprite("assets/targetRange.png"));
		HEALTHBAR_DECAL=new Decal(new Sprite("assets/healthbar.png"));
		COMPUTER_DECAL=new Decal(new Sprite("assets/computerSystem.png"));
		BROKEN_ROVER_DECAL=new Decal(new Sprite("assets/brokenROVER.png"));
		NADO_DECAL=new Decal(new Sprite("assets/nado.png"));
		SILICON_ROCK_DECAL=new Decal(new Sprite("assets/siliconPiece.png"));
		PETAL_STORM_DECAL=new Decal(new Sprite("assets/petalstorm_icon.png"));
		ROVER_DECAL=new Decal(new Sprite("assets/ROVER.png"));
		X_X_DECAL=new Decal(new Sprite("assets/X.X.png"));
		X_X_UNCHARGED_DECAL=new Decal(new Sprite("assets/X.X_uncharged.png"));
		LATER_THAT_NIGHT_DECAL=new Decal(new Sprite("assets/LaterThatNight.png"));
		SLEEP_DECAL=new Decal(new Sprite("assets/sleep.png"));
		SEED_DECAL=new Decal(new Sprite("assets/seed.png"));
		TREE_DECAL=new Decal(new Sprite("assets/tree.png"));
		SANDWORM_DECAL=new Decal(new Sprite("assets/sandworm.png"));
		MOTH_DECAL=new Decal(new Sprite("assets/moth.png"));
		SNAKE_DECAL=new Decal(new Sprite("assets/snake.png"));
		FLASH_FLOOD_DECAL=new Decal(new Sprite("assets/flashflood_icon.png"));
		SUNNY_DAY_DECAL=new Decal(new Sprite("assets/sunny_day.png"));
		FIRESTORM_DECAL=new Decal(new Sprite("assets/firestorm.png"));
		SOLARFLARE_DECAL=new Decal(new Sprite("assets/solarflare.png"));
		HP_REGEN_DECAL=new Decal(new Sprite("assets/hpregen.png"));
		SLOWED_DECAL=new Decal(new Sprite("assets/slowed.png"));
		SPEED_DECAL=new Decal(new Sprite("assets/speedup.png"));
		HIDDEN_DECAL=new Decal(new Sprite("assets/hidden.png"));
		BOOK_DECAL=new Decal(new Sprite("assets/book2.png"));
		BOOK2_DECAL=new Decal(new Sprite("assets/book.png"));
		LAUNCHPAD_CLOSED_DECAL=new Decal(new Sprite("assets/launchpad.png"));
		LAUNCHPAD_HALF1_DECAL=new Decal(new Sprite("assets/launchpad_half1.png"));
		LAUNCHPAD_HALF2_DECAL=new Decal(new Sprite("assets/launchpad_half2.png"));
		LAUNCHPAD_DECAL=new Decal(new Sprite("assets/launchpad_back.png"));
		Y_Y_DECAL=new Decal(new Sprite("assets/Y.Y.png"));
		PETRIFY_DECAL=new Decal(new Sprite("assets/petrify.png"));
		A_A_DECAL=new Decal(new Sprite("assets/A.A.png"));
		A_A_RECHARGE_DECAL=new Decal(new Sprite("assets/A.A_recharge.png"));
		ENERGYBALL_DECAL=new Decal(new Sprite("assets/energyball.png"));
		TOUCHSCREEN_DECAL=new Decal(new Sprite("assets/touchcontroller.png"));

		playerAnim->spr=PLAYER_DECAL;
		playerAnimRight->spr=PLAYER_DECAL;
		playerAnimLeft->spr=PLAYER_DECAL;
		playerAnimLeft->flipped=true;
		playerAnimDown->spr=PLAYER_DECAL;
		playerAnimWalkUp->spr=PLAYER_DECAL;
		playerAnimWalkDown->spr=PLAYER_DECAL;
		playerAnimWalkRight->spr=PLAYER_DECAL;
		playerAnimWalkLeft->spr=PLAYER_DECAL;
		playerAnimWalkLeft->flipped=true;
		playerAnimPlantUp->spr=PLAYER_DECAL;
		playerAnimPlantDown->spr=PLAYER_DECAL;
		playerAnimPlantRight->spr=PLAYER_DECAL;
		playerAnimPlantLeft->spr=PLAYER_DECAL;
		playerAnimPlantLeft->flipped=true;
		playerAnim->frames.push_back({64,0});
		playerAnimRight->frames.push_back({32,0});
		playerAnimLeft->frames.push_back({32,0});
		playerAnimDown->frames.push_back({0,0});
		playerAnimWalkUp->frames.push_back({0,96});
		playerAnimWalkUp->frames.push_back({32,96});
		playerAnimWalkUp->frames.push_back({64,96});
		playerAnimWalkRight->frames.push_back({0,64});
		playerAnimWalkLeft->frames.push_back({0,64});
		playerAnimWalkRight->frames.push_back({32,64});
		playerAnimWalkLeft->frames.push_back({32,64});
		playerAnimWalkRight->frames.push_back({64,64});
		playerAnimWalkLeft->frames.push_back({64,64});
		playerAnimWalkDown->frames.push_back({0,32});
		playerAnimWalkDown->frames.push_back({32,32});
		playerAnimWalkDown->frames.push_back({64,32});
		playerAnimPlantUp->frames.push_back({64,128});
		playerAnimPlantRight->frames.push_back({32,128});
		playerAnimPlantLeft->frames.push_back({32,128});
		playerAnimPlantDown->frames.push_back({0,128});
		current_playerAnim=playerAnimWalkDown;

		const int nodeAnimationSkipFrames=12;

		POWER_HAILSTORM_ANIMATION->spr=POWER_HAILSTORM_DECAL;
		POWER_HAILSTORM_ANIMATION->frames.push_back({0,0});
		POWER_HAILSTORM_ANIMATION->frames.push_back({32,0});
		POWER_HAILSTORM_ANIMATION->frames.push_back({64,0});
		POWER_HAILSTORM_ANIMATION->skip_frames=nodeAnimationSkipFrames;
		POWER_HURRICANE_ANIMATION->spr=POWER_HURRICANE_DECAL;
		POWER_HURRICANE_ANIMATION->frames.push_back({0,0});
		POWER_HURRICANE_ANIMATION->frames.push_back({32,0});
		POWER_HURRICANE_ANIMATION->frames.push_back({64,0});
		POWER_HURRICANE_ANIMATION->skip_frames=nodeAnimationSkipFrames;
		POWER_METEOR_SHOWER_ANIMATION->spr=POWER_METEOR_SHOWER_DECAL;
		POWER_METEOR_SHOWER_ANIMATION->frames.push_back({0,0});
		POWER_METEOR_SHOWER_ANIMATION->frames.push_back({32,0});
		POWER_METEOR_SHOWER_ANIMATION->frames.push_back({64,0});
		POWER_METEOR_SHOWER_ANIMATION->skip_frames=nodeAnimationSkipFrames;
		POWER_METEOR_STORM_ANIMATION->spr=POWER_METEOR_STORM_DECAL;
		POWER_METEOR_STORM_ANIMATION->frames.push_back({0,0});
		POWER_METEOR_STORM_ANIMATION->frames.push_back({32,0});
		POWER_METEOR_STORM_ANIMATION->frames.push_back({64,0});
		POWER_METEOR_STORM_ANIMATION->skip_frames=nodeAnimationSkipFrames;
		POWER_SNOWSTORM_ANIMATION->spr=POWER_SNOWSTORM_DECAL;
		POWER_SNOWSTORM_ANIMATION->frames.push_back({0,0});
		POWER_SNOWSTORM_ANIMATION->frames.push_back({32,0});
		POWER_SNOWSTORM_ANIMATION->frames.push_back({64,0});
		POWER_SNOWSTORM_ANIMATION->skip_frames=nodeAnimationSkipFrames;
		CONSUME_SNACK_ANIMATION->spr=CONSUME_SNACK_DECAL;
		CONSUME_SNACK_ANIMATION->frames.push_back({0,0});
		CONSUME_SNACK_ANIMATION->frames.push_back({32,0});
		CONSUME_SNACK_ANIMATION->frames.push_back({64,0});
		CONSUME_SNACK_ANIMATION->skip_frames=nodeAnimationSkipFrames;
		CONSUME_MEAL_ANIMATION->spr=CONSUME_MEAL_DECAL;
		CONSUME_MEAL_ANIMATION->frames.push_back({0,0});
		CONSUME_MEAL_ANIMATION->frames.push_back({32,0});
		CONSUME_MEAL_ANIMATION->frames.push_back({64,0});
		CONSUME_MEAL_ANIMATION->skip_frames=nodeAnimationSkipFrames;
		NADO_ANIMATION->spr=NADO_DECAL;
		for (int i=0;i<8;i++) {
			NADO_ANIMATION->frames.push_back({i*32,0});
		}
		CONSUME_MEAL_ANIMATION->skip_frames=3;
		PETAL_STORM_ANIMATION->spr=PETAL_STORM_DECAL;
		for (int i=0;i<3;i++) {
			PETAL_STORM_ANIMATION->frames.push_back({i*32,0});
		}
		PETAL_STORM_ANIMATION->skip_frames=nodeAnimationSkipFrames;
		SLEEP_ANIMATION->spr=SLEEP_DECAL;
		for (int i=0;i<3;i++) {
			SLEEP_ANIMATION->frames.push_back({i*32+32,0});
		}
		SLEEP_ANIMATION->skip_frames=15;
		SLEEP_ANIMATION->height=64;
		EMPTY_BED_ANIMATION->spr=SLEEP_DECAL;
		EMPTY_BED_ANIMATION->frames.push_back({0,0});
		EMPTY_BED_ANIMATION->skip_frames=1;
		EMPTY_BED_ANIMATION->height=64;
		POWER_FLASHFLOOD_ANIMATION->spr=FLASH_FLOOD_DECAL;
		for (int i=0;i<3;i++) {
			POWER_FLASHFLOOD_ANIMATION->frames.push_back({i*32,0});
		}
		POWER_FLASHFLOOD_ANIMATION->skip_frames=nodeAnimationSkipFrames;
		POWER_SUNNYDAY_ANIMATION->spr=SUNNY_DAY_DECAL;
		for (int i=0;i<3;i++) {
			POWER_SUNNYDAY_ANIMATION->frames.push_back({i*32,0});
		}
		POWER_SUNNYDAY_ANIMATION->skip_frames=nodeAnimationSkipFrames;
		POWER_FIRESTORM_ANIMATION->spr=FIRESTORM_DECAL;
		for (int i=0;i<3;i++) {
			POWER_FIRESTORM_ANIMATION->frames.push_back({i*32,0});
		}
		POWER_FIRESTORM_ANIMATION->skip_frames=nodeAnimationSkipFrames;
		POWER_SOLARFLARE_ANIMATION->spr=SOLARFLARE_DECAL;
		for (int i=0;i<3;i++) {
			POWER_SOLARFLARE_ANIMATION->frames.push_back({i*32,0});
		}
		POWER_SOLARFLARE_ANIMATION->skip_frames=nodeAnimationSkipFrames;
		POWER_ENERGYBALL_ANIMATION->spr=ENERGYBALL_DECAL;
		for (int i=0;i<3;i++) {
			POWER_ENERGYBALL_ANIMATION->frames.push_back({i*32,0});
		}
		POWER_ENERGYBALL_ANIMATION->skip_frames=nodeAnimationSkipFrames;

		HAILSTORM->playerOwnCount=5;
		HURRICANE->playerOwnCount=1;
		METEOR_RAIN->playerOwnCount=3;
		ENERGY_BALL->playerOwnCount=99;

		if (SKIP_LAUNCHPAD) {
			GAME_STATE=IN_SPACE;
			ROCKET_BOARD_OPTION=true;
			PlayCutscene(cutscene::READY_TO_BOARD);
			fadeOut();
			HAILSTORM->playerOwnCount=12;
			PETAL_STORM->playerOwnCount=12;
			HURRICANE->playerOwnCount=12;
			METEOR_RAIN->playerOwnCount=12;
			METEOR_STORM->playerOwnCount=12;
			SNOWSTORM->playerOwnCount=12;
			FLASH_FLOOD->playerOwnCount=12;
			SUNNY_DAY->playerOwnCount=12;
			FIRESTORM->playerOwnCount=12;
			SOLAR_FLARE->playerOwnCount=12;
			foodCount=16;
			PLAYER_HP=PLAYER_MAXHP=130;
			PLAYER_COORDS[0]=1000;
			PLAYER_COORDS[1]=1000;
		}


		if (SKIP_A_A) {
			GAME_FLAGS[gameflag::INTRUDER_DETECTED]=true;
			GAME_FLAGS[gameflag::DEFEATED_Y_Y]=true;
			GAME_FLAGS[gameflag::DEFEATED_A_A]=true;
			fadeIn();
			StartCutscene(cutscene::ROVER_SAVE);
			SHIP_COLOR=Pixel(0,0,0,0);
		}

		PETAL_STORM->seedProduction=2; //Produce two seeds.
		HURRICANE->seedScatter=2;
		HURRICANE->growSeeds=true;
		METEOR_STORM->burnTrees=true;
		METEOR_RAIN->burnTrees=true;
		METEOR_RAIN->treeBurnChance=0.5;
		SUNNY_DAY->seedProduction=1;
		SUNNY_DAY->seedScatter=1.5;
		SUNNY_DAY->treeSeedChance=0.75;
		FLASH_FLOOD->seedProduction=6;
		SOLAR_FLARE->treeBurnChance=1;
		SOLAR_FLARE->burnTrees=true;
		FIRESTORM->treeBurnChance=1;
		FIRESTORM->burnTrees=true;
		SNOWSTORM->appliesSlow=true;
		HIDE->appliesHide=true;
		HYPERZAP->appliesSpeed=true;
		PETRIFY->appliesPetrification=true;
		MEGA_FANG->dealsPctDmg=true;
		MEGA_FANG->pctDmg=0.49;
		END_OF_THE_CENTURY_BEAM->dealsPctDmg=true;
		END_OF_THE_CENTURY_BEAM->pctDmg=0.96;
		RADIOACTIVE_TRANSMISSION->appliesRadioactive=true;

		LIGHT_STORM->lowPriority=true;

		MOVESET_SPIDEY.push_back(SANDSTORM);
		MOVESET_SPIDEY.push_back(SEED_STORM);
		MOVESET_SPIDEY.push_back(AVALANCHE);

		MOVESET_MOTH.push_back(GUST);
		MOVESET_MOTH.push_back(GUST);
		MOVESET_MOTH.push_back(TORNADO);

		MOVESET_MEGAMOTH.push_back(TORNADO);
		MOVESET_MEGAMOTH.push_back(TORNADO);
		MOVESET_MEGAMOTH.push_back(TORNADO);
		MOVESET_MEGAMOTH.push_back(TORNADO);
		MOVESET_MEGAMOTH.push_back(HIDE);
		MOVESET_MEGAMOTH.push_back(HYPERZAP);
		MOVESET_MEGAMOTH.push_back(POLLINATION);
		MOVESET_MEGAMOTH.push_back(GUST);
		MOVESET_MEGAMOTH.push_back(GUST);

		MOVESET_SANDWORM.push_back(SANDSTORM);
		MOVESET_SANDWORM.push_back(HEAT_WAVE);
		MOVESET_SANDWORM.push_back(DROUGHT);

		MOVESET_SNAKE.push_back(ACID_RAIN);
		MOVESET_SNAKE.push_back(SANDSTORM);

		MOVESET_X_X.push_back(LIGHT_STORM);
		MOVESET_X_X.push_back(SEED_BULLET);
		MOVESET_X_X.push_back(SEED_OF_LIFE);
		MOVESET_X_X.push_back(SEED_BULLET);
		MOVESET_X_X.push_back(SEED_BULLET);
		MOVESET_X_X.push_back(SEED_BULLET);
		MOVESET_X_X.push_back(SUMMON_MINIONS);
		MOVESET_X_X.push_back(SEED_OF_LIFE);
		MOVESET_X_X.push_back(SEED_BULLET);
		MOVESET_X_X.push_back(SEED_BULLET);
		MOVESET_X_X.push_back(SEED_PELLET);

		MOVESET_XMINION.push_back(SEED_PELLET);
		MOVESET_XMINION.push_back(SEED_PELLET);
		MOVESET_XMINION.push_back(SEED_STORM);

		MOVESET_SIDEWINDER.push_back(AVALANCHE);
		MOVESET_SIDEWINDER.push_back(ACID_RAIN);
		MOVESET_SIDEWINDER.push_back(ACID_RAIN);
		MOVESET_SIDEWINDER.push_back(TORNADO);
		MOVESET_SIDEWINDER.push_back(AVALANCHE);
		MOVESET_SIDEWINDER.push_back(ACID_RAIN);
		MOVESET_SIDEWINDER.push_back(GLARE);
		MOVESET_SIDEWINDER.push_back(PETRIFY);
		MOVESET_SIDEWINDER.push_back(MEGA_FANG);
		MOVESET_SIDEWINDER.push_back(MEGA_FANG);

		MOVESET_Y_Y.push_back(METEOR_RAIN);
		MOVESET_Y_Y.push_back(AVALANCHE);
		MOVESET_Y_Y.push_back(LIGHT_STORM);
		MOVESET_Y_Y.push_back(TORNADO);
		MOVESET_Y_Y.push_back(GUST);
		MOVESET_Y_Y.push_back(DROUGHT);
		MOVESET_Y_Y.push_back(HAILSTORM);

		MOVESET_A_A.push_back(END_OF_THE_CENTURY_BEAM);
		MOVESET_A_A.push_back(LIGHT_STORM);
		MOVESET_A_A.push_back(RADIOACTIVE_TRANSMISSION);
		MOVESET_A_A.push_back(LIGHT_STORM);
		MOVESET_A_A.push_back(METEOR_RAIN);
		MOVESET_A_A.push_back(LIGHT_STORM);
		MOVESET_A_A.push_back(LIGHT_STORM);
		MOVESET_A_A.push_back(AVALANCHE);
		MOVESET_A_A.push_back(FIRESTORM);
		MOVESET_A_A.push_back(AVALANCHE);
		MOVESET_A_A.push_back(GUST);
		MOVESET_A_A.push_back(GUST);
		MOVESET_A_A.push_back(GUST);

		COLLECTED_ITEMS.push_back({20,5});
		COLLECTED_ITEMS.push_back({20,6});
		COLLECTED_ITEMS.push_back({19,5});
		COLLECTED_ITEMS.push_back({19,6});

		ENCOUNTER_SPIDEY_1.entities.push_back(new Entity(SPIDEY_DECAL,"Spidey",2,3,80,80,MOVESET_SPIDEY));
		ENCOUNTER_SPIDEY_1.entities.push_back(new Entity(SPIDEY_DECAL,"Spidey",4,4,80,80,MOVESET_SPIDEY));
		ENCOUNTER_SPIDEY_1.entities.push_back(new Entity(SPIDEY_DECAL,"Spidey",6,2,80,80,MOVESET_SPIDEY));
		ENCOUNTER_SPIDEY_1.x=79;
		ENCOUNTER_SPIDEY_1.y=47;
		ENCOUNTER_SPIDEY_1.playerX=4;
		ENCOUNTER_SPIDEY_1.playerY=3;
		ENCOUNTER_SPIDEY_1.map=MAP_1;
		ENCOUNTERS.push_back(ENCOUNTER_SPIDEY_1);
		ENCOUNTER_X_X.entities.push_back(new Entity(X_X_DECAL,"X_X",3,2.5,590,590,MOVESET_X_X,{2,2},true));
		ENCOUNTER_X_X.entities.push_back(new Entity(X_X_DECAL,"X Minion",1,4,0,80,MOVESET_XMINION,{0.7,0.7}));
		ENCOUNTER_X_X.entities.push_back(new Entity(X_X_DECAL,"X Minion",0,2,0,80,MOVESET_XMINION,{0.7,0.7}));
		ENCOUNTER_X_X.entities.push_back(new Entity(X_X_DECAL,"X Minion",4,2,0,80,MOVESET_XMINION,{0.7,0.7}));
		ENCOUNTER_X_X.x=38;
		ENCOUNTER_X_X.y=35;
		ENCOUNTER_X_X.playerX=3;
		ENCOUNTER_X_X.playerY=2;
		ENCOUNTER_X_X.map=MAP_1;
		ENCOUNTER_SANDWORM_1.entities.push_back(new Entity(SANDWORM_DECAL,"Sandworm",3,4,165,165,MOVESET_SANDWORM));
		ENCOUNTER_SANDWORM_1.entities.push_back(new Entity(SANDWORM_DECAL,"Sandworm",6,3,165,165,MOVESET_SANDWORM));
		ENCOUNTER_SANDWORM_1.x=20-4;
		ENCOUNTER_SANDWORM_1.y=73-3.5;
		ENCOUNTER_SANDWORM_1.playerX=4;
		ENCOUNTER_SANDWORM_1.playerY=3.5;
		ENCOUNTER_SANDWORM_1.map=MAP_3;
		//ENCOUNTERS.push_back(ENCOUNTER_X_X); //Activate at beginning of Chapter 2.
		ENCOUNTER_MEGAMOTH.entities.push_back(new Entity(MOTH_DECAL,"Megamoth",3.5,1.75,745,745,MOVESET_MEGAMOTH,{2,2},true));
		ENCOUNTER_MEGAMOTH.entities.push_back(new Entity(MOTH_DECAL,"Moth",2,1,0,120,MOVESET_MOTH));
		ENCOUNTER_MEGAMOTH.entities.push_back(new Entity(MOTH_DECAL,"Moth",6,1,0,120,MOVESET_MOTH));
		ENCOUNTER_MEGAMOTH.entities.push_back(new Entity(MOTH_DECAL,"Moth",4,4,0,120,MOVESET_MOTH));
		ENCOUNTER_MEGAMOTH.x=195-4;
		ENCOUNTER_MEGAMOTH.y=56-3.5;
		ENCOUNTER_MEGAMOTH.playerX=4;
		ENCOUNTER_MEGAMOTH.playerY=0.5;
		ENCOUNTER_MEGAMOTH.map=MAP_4;
		ENCOUNTERS.push_back(ENCOUNTER_MEGAMOTH);
		ENCOUNTER_SNAKEPACK.entities.push_back(new Entity(SNAKE_DECAL,"Snake",1,1,225,225,MOVESET_SNAKE));
		ENCOUNTER_SNAKEPACK.entities.push_back(new Entity(SNAKE_DECAL,"Snake",3,2,0,225,MOVESET_SNAKE));
		ENCOUNTER_SNAKEPACK.entities.push_back(new Entity(SNAKE_DECAL,"Sidewinder",4,3,0,1065,MOVESET_SIDEWINDER,{2,2},true));
		ENCOUNTER_SNAKEPACK.entities.push_back(new Entity(SNAKE_DECAL,"Snake",5,3,225,225,MOVESET_SNAKE));
		ENCOUNTER_SNAKEPACK.entities.push_back(new Entity(SNAKE_DECAL,"Snake",6,1,0,225,MOVESET_SNAKE));
		ENCOUNTER_SNAKEPACK.x=11-4;
		ENCOUNTER_SNAKEPACK.y=5-3.5;
		ENCOUNTER_SNAKEPACK.playerX=4;
		ENCOUNTER_SNAKEPACK.playerY=6;
		ENCOUNTER_SNAKEPACK.map=MAP_5;
		ENCOUNTERS.push_back(ENCOUNTER_SNAKEPACK);
		ENCOUNTER_Y_Y.entities.push_back(new Entity(Y_Y_DECAL,"Y.Y",4-1,3.5-2,895,895,MOVESET_Y_Y,{1,1},true));
		ENCOUNTER_Y_Y.x=1000-4;
		ENCOUNTER_Y_Y.y=1000-3.5;
		ENCOUNTER_Y_Y.playerX=4;
		ENCOUNTER_Y_Y.playerY=6;
		ENCOUNTER_Y_Y.map=MAP_6;
		ENCOUNTER_A_A.entities.push_back(new Entity(A_A_DECAL,"A.A",4-2,0,4096,4096,MOVESET_A_A,{2,2},true));
		ENCOUNTER_A_A.x=1000-4;
		ENCOUNTER_A_A.y=1000-3.5;
		ENCOUNTER_A_A.playerX=4;
		ENCOUNTER_A_A.playerY=6;
		ENCOUNTER_A_A.map=MAP_6;
		//ENCOUNTERS.push_back(ENCOUNTER_Y_Y);


		BASE_OBJECTS["DOME"]=new ObjectLoadInfo(DOME_DECAL);
		BASE_OBJECTS["PLANT"]=new ObjectLoadInfo(PLANT_DECAL);
		BASE_OBJECTS["EXIT"]=new ObjectLoadInfo(NULL);
		BASE_OBJECTS["HAILSTORM_NODE"]=new ObjectLoadInfo(POWER_HAILSTORM_DECAL,POWER_HAILSTORM_ANIMATION);
		BASE_OBJECTS["HURRICANE_NODE"]=new ObjectLoadInfo(POWER_HURRICANE_DECAL,POWER_HURRICANE_ANIMATION);
		BASE_OBJECTS["METEORSHOWER_NODE"]=new ObjectLoadInfo(POWER_METEOR_SHOWER_DECAL,POWER_METEOR_SHOWER_ANIMATION);
		BASE_OBJECTS["METEORSTORM_NODE"]=new ObjectLoadInfo(POWER_METEOR_STORM_DECAL,POWER_METEOR_STORM_ANIMATION);
		BASE_OBJECTS["SNOWSTORM_NODE"]=new ObjectLoadInfo(POWER_SNOWSTORM_DECAL,POWER_SNOWSTORM_ANIMATION);
		BASE_OBJECTS["PETALSTORM_NODE"]=new ObjectLoadInfo(PETAL_STORM_DECAL,PETAL_STORM_ANIMATION);
		BASE_OBJECTS["SUNNYDAY_NODE"]=new ObjectLoadInfo(SUNNY_DAY_DECAL,POWER_SUNNYDAY_ANIMATION);
		BASE_OBJECTS["FLASHFLOOD_NODE"]=new ObjectLoadInfo(FLASH_FLOOD_DECAL,POWER_FLASHFLOOD_ANIMATION);
		BASE_OBJECTS["FIRESTORM_NODE"]=new ObjectLoadInfo(FIRESTORM_DECAL,POWER_FIRESTORM_ANIMATION);
		BASE_OBJECTS["SOLARFLARE_NODE"]=new ObjectLoadInfo(SOLARFLARE_DECAL,POWER_SOLARFLARE_ANIMATION);
		BASE_OBJECTS["COMPUTER"]=new ObjectLoadInfo(COMPUTER_DECAL);
		BASE_OBJECTS["BROKEN_ROVER"]=new ObjectLoadInfo(BROKEN_ROVER_DECAL);
		BASE_OBJECTS["NADO"]=new ObjectLoadInfo(NADO_DECAL,NADO_ANIMATION,Pixel(153, 137, 75,230));
		BASE_OBJECTS["SILICON_PIECE"]=new ObjectLoadInfo(SILICON_ROCK_DECAL);
		BASE_OBJECTS["LAUNCHPAD"]=new ObjectLoadInfo(LAUNCHPAD_DECAL);
		BASE_OBJECTS["LAUNCHPAD_HALF1"]=new ObjectLoadInfo(LAUNCHPAD_HALF1_DECAL);
		BASE_OBJECTS["LAUNCHPAD_HALF2"]=new ObjectLoadInfo(LAUNCHPAD_HALF2_DECAL);
		BASE_OBJECTS["Y_Y"]=new ObjectLoadInfo(Y_Y_DECAL);

		for (int i=0;i<WEATHER_POWER_COUNT;i++) {
			PREV_PLAYERSTATE->weatherPowerCounts.push_back(WEATHER_POWERS[i]->playerOwnCount);
		}

		Zone*SILICON_DEPOSIT_ZONE = new Zone({109,7},{26,9},{0,0},{WIDTH,HEIGHT},{0,0},{WIDTH,HEIGHT},{1,1},{3,3},{-30,-3},{30,3},Pixel(133, 98, 66,180),Pixel(220, 120, 90,230),300,Pixel(87, 78, 69,64));

		ZONES.push_back(SILICON_DEPOSIT_ZONE);


		ResetTerminal(STORY_TEXT2);


		for (int i=0;i<MAX_PIXELS;i++) {
			pixels[i]=new effect::Pixel();
		}
		for (int i=0;i<150;i++) {
			starpixels[i]=new effect::Pixel();
		}

		if (SKIP_LAUNCHPAD) {
			LoadMap(MAP_6);
		} else 
		if (SKIP_STORYBOOK) {
			LoadMap(MAP_5);
		} else {
			LoadMap(STARTING_MAP);
		}
		return true;
	}

	void GetAnyKeyPress() override { 
		if (messageBoxVisible) {
			if (messageBoxCursor!=messageBoxRefText.length()) {
				while (messageBoxCursor<messageBoxRefText.length()) {
					advanceMessageBox();
				}
			} else {
				messageBoxVisible=false;
			}
			return;
		}
		if (GetMouse(0).bPressed) {
			USE_TOUCH_CONTROLS=true;
		} else {
			USE_TOUCH_CONTROLS=false;
		}
		if (!UpPressed()&&!DownPressed()&&!LeftPressed()&&!RightPressed()) {
			ActionButtonPress();
		} 
		switch (GAME_STATE) {
			case CUTSCENE_1:{
				if (textInd>=CONSOLE_REF_TEXT.length()) {
					fadeOut();
				}
			}break;
			case CUTSCENE_4:{
				if (textInd>=CONSOLE_REF_TEXT.length()) {
					GAME_STATE=CUTSCENE_4_DONE;
					fadeOut();
				}
			}break;
			case GAME_OVER_TERMINAL:{
				if (textInd>=CONSOLE_REF_TEXT.length()) {
					//audioFadeOut();
					queueBGMPlayback=false;
					if (!GAME_FLAGS[gameflag::ROCKET_LAUNCH_READY]) {
						playMusic(&SONG_EXPLORE);
					} else {
						playMusic(&SONG_DOME);
					}
					fadeOut();
				}
			}break;
			case CUTSCENE_5:{
				if (textInd>=CONSOLE_REF_TEXT.length()) {
					fadeOut();
				}
			}break;
			case CUTSCENE_6:{
				if (textInd>=CONSOLE_REF_TEXT.length()) {
					fadeOut();
				}
			}break;
		}
	}

	void ActionButtonPress() {
		if (GetMouse(0).bPressed) {
			MOUSE_DOWN=true;
			MOUSE_PRESSED_DOWN=true;
			if (GetMouseX()<128&&GetMouseY()>HEIGHT-128) {
				return;
			}
		}
		if (playerCanMove()) {
			switch (GAME_STATE) {
				case GAMEWORLD:{
					if (PLAYER_COORDS[0]>=8&&PLAYER_COORDS[0]<12&&
					PLAYER_COORDS[1]>=2&&PLAYER_COORDS[1]<6) {
						//cout<<"You are standing over plant "<<getPlantId((int)PLAYER_COORDS[0],(int)PLAYER_COORDS[1])<<" in state "<<getPlantStatus((int)PLAYER_COORDS[0],(int)PLAYER_COORDS[1]);
						if (getPlantStatus((int)PLAYER_COORDS[0],(int)PLAYER_COORDS[1])==2) {
							setPlantStatus((int)PLAYER_COORDS[0],(int)PLAYER_COORDS[1],0);
							foodCount++;
							PlaySound(&SOUND_SELECT,false,0.6);
						}
					}
				}break;
			}
		}
		if (GAME_STATE==CUTSCENE_3) {
			switch (MAIN_MENU_SELECTION) {
				case 0:{
					PlaySound(&SOUND_SELECT);
					fadeOut();
					audioFadeOut();
				}break;
				case 1:{
					SOUND_IS_MUTED=!SOUND_IS_MUTED;
					if (SOUND_IS_MUTED) {
						MENU_OPTIONS[1]="UNMUTE SOUND";
						engine.SetOutputVolume(0);
					} else {
						MENU_OPTIONS[1]="MUTE SOUND";
						engine.SetOutputVolume(0.6);
					}
				}break;
				case 2:{
					END_THE_GAME=true;
				}break;
			}
		}
		switch (BATTLE_STATE) {
			case battle::PLAYER_SELECTION:{
				if (BATTLE_CARD_SELECTION->playerOwnCount>0) {
					BATTLE_STATE=battle::PLAYER_TARGET_SELECTION;
					if (!(BATTLE_CARD_SELECTION->name.compare("Meal")==0||BATTLE_CARD_SELECTION->name.compare("Snack")==0)) {
						if (PLAYER_SELECTED_TARGET==-1||CURRENT_ENCOUNTER.entities[PLAYER_SELECTED_TARGET]->hp<=0) {
							for (int i=0;i<CURRENT_ENCOUNTER.entities.size();i++) {
								Entity*ent = CURRENT_ENCOUNTER.entities[i];
								if (!ent->ally&&ent->hp>0) {
									PLAYER_SELECTED_TARGET=i;
									break;
								}
							}
						}
					} else {
						PLAYER_SELECTED_TARGET=-2;
						setupBattleTurns();
					}
					PlaySound(&SOUND_SELECT);
				} else {
					PlaySound(&SOUND_ROBOTICNOISE);
				}
			}break;
			case battle::PLAYER_TARGET_SELECTION:{
				setupBattleTurns();
				PlaySound(&SOUND_SELECT);
			}break;
		}
		switch (CURRENT_CUTSCENE) {
			case cutscene::NODE_COLLECT_CUTSCENE:{
				EndCutscene();
			}break;
			case cutscene::INPUT_USERNAME:{
				if (TERMINAL_SELECTED_CHAR<26&&TERMINAL_INPUT.length()<MAX_TERMINAL_NAME_LENGTH) {
					TERMINAL_INPUT+=(char)('A'+TERMINAL_SELECTED_CHAR);
					if (TERMINAL_INPUT.length()==MAX_TERMINAL_NAME_LENGTH) {
						TERMINAL_SELECTED_CHAR=27;
					}
					PlaySound(&SOUND_CHANGE_OPTION);
				} else 
				if (TERMINAL_SELECTED_CHAR==26&&TERMINAL_INPUT.length()>0) {
					TERMINAL_INPUT.erase(TERMINAL_INPUT.length()-1);
					PlaySound(&SOUND_SELECT,false,0.3);
				} else
				if (TERMINAL_SELECTED_CHAR==27&&TERMINAL_INPUT.length()>0) {
					PLAYER_NAME=TERMINAL_INPUT;
					PlayCutscene(cutscene::GO_OUTSIDE);
					PlaySound(&SOUND_SELECT);
				}
			}break;
			case cutscene::READY_TO_BOARD:{
				if (ROCKET_BOARD_OPTION) {
					fadeOut();
				} else {
					EndCutscene();
					PLAYER_COORDS[0]=31;
					PLAYER_COORDS[1]=37;
				}
			}break;
		}
	}

	bool OnUserUpdate(float fElapsedTime) override
	{
		elapsedTime+=fElapsedTime;
		if (GetMouse(0).bReleased) {
			MOUSE_DOWN=false;
			MOUSE_RELEASED=true;
		}
		while (elapsedTime>TARGET_RATE) {
			elapsedTime-=TARGET_RATE;
			updateGame();
			rand();
		}
		if (GetKey(F1).bPressed) {
			//ConsoleShow(F1,false); //Disable for official release.
		}
		if (playerCanMove()) {
			if (RightPressed()) {
				changeAnimation(playerAnimWalkRight);
			}
			if (LeftPressed()) {
				changeAnimation(playerAnimWalkLeft);
			}
			if (UpPressed()) {
				changeAnimation(playerAnimWalkUp);
			}
			if (DownPressed()) {
				changeAnimation(playerAnimWalkDown);
			}
			if (!RightHeld()&&
				!LeftHeld()&&
				!UpHeld()&&
				!DownHeld()) {
				if (RightReleased()) {
					changeAnimation(playerAnimRight);
				}
				if (LeftReleased()) {
					changeAnimation(playerAnimLeft);
				}
				if (UpReleased()) {
					changeAnimation(playerAnim);
				}
				if (DownReleased()) {
					changeAnimation(playerAnimDown);
				}
			}
		} else
		if (IN_BATTLE_ENCOUNTER&&!messageBoxVisible) {
			switch (BATTLE_STATE) {
				case battle::PLAYER_SELECTION:{
					if (RightPressed()) {
						BATTLE_CARD_SELECTION_IND=(BATTLE_CARD_SELECTION_IND+1)%availablePowers.size();
						BATTLE_CARD_SELECTION=availablePowers[BATTLE_CARD_SELECTION_IND];
						PlaySound(&SOUND_CHANGE_OPTION);
					} 
					if (LeftPressed()) {
						if (--BATTLE_CARD_SELECTION_IND<0) {
							BATTLE_CARD_SELECTION_IND=availablePowers.size()-1;
						}
						BATTLE_CARD_SELECTION=availablePowers[BATTLE_CARD_SELECTION_IND];
						PlaySound(&SOUND_CHANGE_OPTION);
					}
				}break;
				case battle::PLAYER_TARGET_SELECTION:{
					if (RightPressed()) {
						while (true) {
							PLAYER_SELECTED_TARGET=(PLAYER_SELECTED_TARGET+1)%CURRENT_ENCOUNTER.entities.size();
							if (CURRENT_ENCOUNTER.entities[PLAYER_SELECTED_TARGET]->hp>0) {
								break;
							}
						}
						PlaySound(&SOUND_CHANGE_OPTION);
					} 
					if (UpPressed()) {
						PLAYER_SELECTED_TARGET=-1;
						//std::cout<<"Battle State is "<<BATTLE_STATE<<" (1)\n";
						BATTLE_STATE=battle::PLAYER_SELECTION;
					}
					if (LeftPressed()) {
						while (true) {
							if (--PLAYER_SELECTED_TARGET<0) {
								PLAYER_SELECTED_TARGET=CURRENT_ENCOUNTER.entities.size()-1;
							}
							if (CURRENT_ENCOUNTER.entities[PLAYER_SELECTED_TARGET]->hp>0) {
								break;
							}
						}
						PlaySound(&SOUND_CHANGE_OPTION);
					}
				}break;
			}
		} else {
			if (GAME_STATE==CUTSCENE_3) {
				if (LeftPressed()||UpPressed()) {
					MAIN_MENU_SELECTION--;
					if (MAIN_MENU_SELECTION<0) {
						MAIN_MENU_SELECTION=2;
					}
					PlaySound(&SOUND_CHANGE_OPTION);
				}
				if (DownPressed()||RightPressed()) {
					MAIN_MENU_SELECTION=(MAIN_MENU_SELECTION+1)%3;
					PlaySound(&SOUND_CHANGE_OPTION);
				}
			}
			switch (CURRENT_CUTSCENE) {
				case cutscene::INPUT_USERNAME:{
					if (UpPressed()) {
						TERMINAL_SELECTED_CHAR=TERMINAL_SELECTED_CHAR-7;
						if (TERMINAL_SELECTED_CHAR<0) {
							TERMINAL_SELECTED_CHAR+=28;
						}
						PlaySound(&SOUND_MSG);
					}
					if (RightPressed()) {
						if ((TERMINAL_SELECTED_CHAR+1)%7==0) {
							TERMINAL_SELECTED_CHAR-=6;
						} else {
							TERMINAL_SELECTED_CHAR++;
						}
						PlaySound(&SOUND_MSG);
					}
					if (LeftPressed()) {
						if ((TERMINAL_SELECTED_CHAR-1)%7==6||TERMINAL_SELECTED_CHAR-1<0) {
							TERMINAL_SELECTED_CHAR+=6;
						} else {
							TERMINAL_SELECTED_CHAR--;
						}
						PlaySound(&SOUND_MSG);
					}
					if (DownPressed()) {
						if (TERMINAL_SELECTED_CHAR+7>=28) {
							TERMINAL_SELECTED_CHAR=(TERMINAL_SELECTED_CHAR+7)%28;
						} else {
							TERMINAL_SELECTED_CHAR+=7;
						}
						PlaySound(&SOUND_MSG);
					}
				}break;
				case cutscene::READY_TO_BOARD:{
					if (UpPressed()||DownPressed()) {
						ROCKET_BOARD_OPTION=!ROCKET_BOARD_OPTION;
					}
				}break;
			}
		}
		SetDrawTarget(nullptr);
		Clear(BLANK);
		SetDrawTarget(1);
		Clear(BLANK);
		SetDrawTarget(2);
		drawGame();
		MOUSE_PRESSED_DOWN=false;
		MOUSE_RELEASED=false;
		// called once per frame
		return !END_THE_GAME;
	}

	void fadeOutCompleted() {
		switch (GAME_STATE) {
			case CUTSCENE_1:{
				GAME_STATE=CUTSCENE_2;
				PlayCutscene(cutscene::PAN_DOME);
				fadeIn();
			}break;
			case CUTSCENE_2:{
				fadeIn();
				PlayCutscene(cutscene::CUTSCENE_4);
				GAME_STATE=GAMEWORLD;
			}break;
			case CUTSCENE_3:{
				GAME_STATE=CUTSCENE_1;
				fadeIn();
				stopMusic();
				ResetTerminal(STORY_TEXT1);
			}break;
			case CUTSCENE_4:{
				fadeIn();
			}break;
			case CUTSCENE_4_DONE:{
				GAME_STATE=GAMEWORLD;
				PlayCutscene(cutscene::GET_SOME_REST);	
				fadeIn();
			}break;
			case LATER_THAT_NIGHTFADEIN:{
				fadeIn();
			}break;
			case LATER_THAT_NIGHTFADEOUT:{
				fadeIn();
				GAME_STATE=GAMEWORLD;
				PlayCutscene(cutscene::IN_BED);
				PLAYER_HP=PLAYER_MAXHP;
			}break;
			case GAME_OVER:{
				GAME_STATE=GAME_OVER_TERMINAL;
				fadeIn();
			}break;
			case GAME_OVER_TERMINAL:{
				LoadPreviousPlayerState();
				if (GAME_FLAGS[gameflag::BOARDED_ROCKET]) {
					GAME_STATE=IN_SPACE;
				} else {
					GAME_STATE=GAMEWORLD;
				}
				fadeIn();
			}break;
			case CUTSCENE_5:{
				EndCutscene();
				fadeIn();
				GAME_STATE=GAMEWORLD;
				DisplayMessageBox(36);
			}break;
			case CUTSCENE_6:{
				PlayCutscene(cutscene::LAUNCHPAD_OPEN);
				fadeIn();
				GAME_STATE=GAMEWORLD;
			}break;
			case FIN:{
				GAME_STATE=THANKS;
				fadeIn();
			}break;
		}
		switch (CURRENT_CUTSCENE) {
			case cutscene::TRANSITION_CUTSCENE:{
				if (GAME_FLAGS[gameflag::ROCKET_LAUNCH_READY]) {
					LoadMap(MAP_6);
					PLAYER_COORDS[0]=40.5;
					PLAYER_COORDS[1]=37.5;
					updatePlayerState();
					fadeIn();
					EndCutscene();
				} else 
				if (GAME_FLAGS[gameflag::NEXT_COORDS2]) {
					LoadMap(MAP_5);
					PLAYER_COORDS[0]=40.5;
					PLAYER_COORDS[1]=37.5;
					updatePlayerState();
					fadeIn();
					EndCutscene();
				} else 
				if (GAME_FLAGS[gameflag::CHECK_ROVER_2]) {
					LoadMap(MAP_4);
					PLAYER_COORDS[0]=40.5;
					PLAYER_COORDS[1]=37.5;
					updatePlayerState();
					fadeIn();
					EndCutscene();
				} else 
				if (GAME_FLAGS[gameflag::CHECK_ROVER]) {
					LoadMap(MAP_3);
					PLAYER_COORDS[0]=40.5;
					PLAYER_COORDS[1]=37.5;
					updatePlayerState();
					fadeIn();
					EndCutscene();
				} else {
					LoadMap(MAP_1);
					PLAYER_COORDS[0]=40.5;
					PLAYER_COORDS[1]=37.5;
					updatePlayerState();
					fadeIn();
					EndCutscene();
				}
				if (!GAME_FLAGS[gameflag::VISIT_BROKEN_ROVER]) {
					GAME_FLAGS[gameflag::VISIT_BROKEN_ROVER]=true;
					PlayCutscene(cutscene::WALK_TO_ROVER);
				}
				if (!GAME_FLAGS[gameflag::ROCKET_LAUNCH_READY]) {
					playMusic(&SONG_EXPLORE);
				} else {
					playMusic(&SONG_DOME);
				}
			}break;
			case cutscene::TRANSITION_CUTSCENE_2:{
				if (GAME_FLAGS[gameflag::ROCKET_LAUNCH_READY]&&!GAME_FLAGS[gameflag::COLLECTED_ITEMS_IN_DOME]) {
					for (int i=0;i<COLLECTED_ITEMS.size();i++) {
						if (COLLECTED_ITEMS[i].x==19&&COLLECTED_ITEMS[i].y==5) {
							COLLECTED_ITEMS.erase(COLLECTED_ITEMS.begin()+i--);
						} else
						if (COLLECTED_ITEMS[i].x==19&&COLLECTED_ITEMS[i].y==6) {
							COLLECTED_ITEMS.erase(COLLECTED_ITEMS.begin()+i--);
						}
					}
					GAME_FLAGS[gameflag::COLLECTED_ITEMS_IN_DOME]=true;
				}
				if (GAME_FLAGS[gameflag::REST_IN_DOME]&&!GAME_FLAGS[gameflag::SLEEP]) {
					for (int i=0;i<COLLECTED_ITEMS.size();i++) {
						if (COLLECTED_ITEMS[i].x==20&&COLLECTED_ITEMS[i].y==5) {
							COLLECTED_ITEMS.erase(COLLECTED_ITEMS.begin()+i--);
						} else
						if (COLLECTED_ITEMS[i].x==20&&COLLECTED_ITEMS[i].y==6) {
							COLLECTED_ITEMS.erase(COLLECTED_ITEMS.begin()+i--);
						}
					}
				}
				LoadMap(MAP_2);
				TeleportToMapFileCoords(8,17);
				updatePlayerState();
				fadeIn();
				EndCutscene();
				playMusic(&SONG_DOME);
				if (GAME_FLAGS[gameflag::CHECK_ROVER_3]&&!GAME_FLAGS[gameflag::STORY_REVIEW]) {
					GAME_FLAGS[gameflag::STORY_REVIEW]=true;
					//20,2
					StartCutscene(cutscene::STORY_REVIEW);
					CUTSCENE_OBJS[0]=CreateObject({PLAYER_COORDS[0],PLAYER_COORDS[1]},PLAYER_DECAL,playerAnim,true);
					CUTSCENE_OBJS[1]=CreateObject({20,2},SLEEP_DECAL,EMPTY_BED_ANIMATION,true);
				} else
				if (GAME_FLAGS[gameflag::REST_IN_DOME]&&!GAME_FLAGS[gameflag::SLEEP]) {
					GAME_FLAGS[gameflag::SLEEP]=true;
					//20,2
					GAME_STATE=LATER_THAT_NIGHTFADEIN;
					fadeOutCompleted();
					playMusic(&SONG_DOME,true,0.4);
				} else
				if (!GAME_FLAGS[gameflag::CHECK_ROVER]&&GAME_FLAGS[gameflag::FIRST_ENCOUNTER_X_X]) {
					GAME_FLAGS[gameflag::CHECK_ROVER]=true;
					DisplayMessageBox(26);
				}
			}break;
			case cutscene::DISPLAY_VOLCANIC_AREA:{
				TeleportToMapFileCoords(13,122);
				fadeIn();
			}break;
			case cutscene::REPAIR_ROVER_1:{
				ResetTerminal(STORY_TEXT3);
				GAME_STATE=CUTSCENE_4;
				REPAIRED_ROVERS.push_back({33,35});
				fadeIn();
			}break;
			case cutscene::END_VOLCANIC_AREA_CUTSCENE:{
				PLAYER_COORDS[0]=33;
				PLAYER_COORDS[1]=34;
				ClearAllTemporaryObjects();
				fadeIn();
			}break;
			case cutscene::MAP_TRANSITION:{
				LoadMap(MAP_4);
				fadeIn();
			}break;
			case cutscene::CHECK_COORDS_3:{
				GAME_STATE=CUTSCENE_5;
				ResetTerminal(STORY_TEXT6);
				fadeIn();
			}break;
			case cutscene::STORY_REVIEW:{
				GAME_STATE=DISPLAY_BOOK;
				playMusic(&SONG_MAIN,true,0.95);
				fadeIn();
				PlayCutscene(cutscene::STORY_REVIEW2);
			}break;
			case cutscene::STORY_REVIEW2:{
				GAME_STATE=GAMEWORLD;
				GAME_FLAGS[gameflag::NEXT_COORDS2]=true;
				LoadMap(MAP_2);
				updatePlayerState();
				StartCutscene(cutscene::FINAL_REVIEW);
				playMusic(&SONG_EXPLORE);
				fadeIn();
			}break;
			case cutscene::PREPARE_SEQUENCE:{
				GAME_STATE=CUTSCENE_6;
				ResetTerminal(STORY_TEXT7);
				fadeIn();
				EndCutscene();
			}break;
			case cutscene::LAUNCHPAD_PREPARE:{
				GAME_STATE=GAMEWORLD;
				LoadMap(MAP_6);
				updatePlayerState();
				fadeIn();
				EndCutscene();
			}break;
			case cutscene::READY_TO_BOARD:{
				GAME_STATE=IN_SPACE;
				TIMER=0;
				fadeIn();
				EndCutscene();
			}break;
			case cutscene::ROVER_SAVE:{
				GAME_STATE=FIN;
				TIMER=0;
				fadeIn();
				EndCutscene();
			}break;
		}
	}

	void fadeInCompleted() {
		switch (GAME_STATE) {
			case LATER_THAT_NIGHTFADEIN:{
				TIMER=0;
				GAME_STATE=LATER_THAT_NIGHTWAIT;
			}break;
			case IN_SPACE:{
				PLAYER_COORDS[0]=1000;
				PLAYER_COORDS[1]=1000;
				STAR_POS={0,0};
				STAR_SIZE={WIDTH,HEIGHT};
				for (int i=0;i<150;i++) {
					int brightness=rand()%60+195;
					starpixels[i]->r=brightness;
					starpixels[i]->g=brightness;
					starpixels[i]->b=brightness;
					starpixels[i]->pos={WIDTH/2,HEIGHT/2-24};
					starpixels[i]->spd={range(-4,4),range(-3,3)};
					float size = range(1,2);
					starpixels[i]->size={size,size};
					starpixels[i]->a=255;
				}
				TIMER=0;
			}break;
		}
		switch (CURRENT_CUTSCENE) {
			case cutscene::DISPLAY_VOLCANIC_AREA:{
				CUTSCENE_FLAGS[0]=true;
			}break;
			case cutscene::END_VOLCANIC_AREA_CUTSCENE:{
				DisplayMessageBox(10);
				EndCutscene();
			}break;
			case cutscene::MAP_TRANSITION:{
				EndCutscene();
			}break;
		}
	}

	void StartCutscene(cutscene::CUTSCENE scene) {
		PlayCutscene(scene);
	}

	void RunCutscene(cutscene::CUTSCENE scene) {
		PlayCutscene(scene);
	}

	void PlayCutscene(cutscene::CUTSCENE scene) {
		CURRENT_CUTSCENE=scene;
		switch (scene) {
			case cutscene::PAN_DOME:{
				PLAYER_COORDS[0]=14;
				PLAYER_COORDS[1]=35+(64/2/32);
			}break;
			case cutscene::PAUSE_TO_CUTSCENE_3:{
				ResetTerminal(STORY_TEXT2);
			}break;
			case cutscene::CUTSCENE_4:{
				LoadMap(MAP_2);
				playMusic(&SONG_DOME);
				PLAYER_COORDS[0]=16;
				PLAYER_COORDS[1]=6;
				updatePlayerState();
			}break;
			case cutscene::RAINING_IN_DOME:{
				applyPixelEffect(HURRICANE,GetMapFileCoords(7,11),1);
				CUTSCENE_TIMER=0;
			}break;
			case cutscene::WALK_TO_ROVER:{
				CUTSCENE_OBJS[0]=CreateObject({PLAYER_COORDS[0],PLAYER_COORDS[1]},PLAYER_DECAL,{32,0},{32,32},true);
				CUTSCENE_OBJS[0]->flipped=true;
			}break;
			case cutscene::REPAIR_ROVER_1:{
				CUTSCENE_OBJS[0]=CreateObject({PLAYER_COORDS[0],PLAYER_COORDS[1]},PLAYER_DECAL,{64,0},{32,32},true);
			}break;
			case cutscene::GET_SOME_REST:{
				DisplayMessageBox(18);
			}break;
			case cutscene::IN_BED:{
				CUTSCENE_OBJS[0]=CreateObject({20,2},SLEEP_DECAL,SLEEP_ANIMATION,true);
			}break;
			case cutscene::INVESTIGATE_X_X:{
				CUTSCENE_OBJS[0]->anim=EMPTY_BED_ANIMATION;
				CUTSCENE_OBJS[1]=CreateObject({19,3},PLAYER_DECAL,{32,0},{32,32},true);
				CUTSCENE_OBJS[1]->flipped=true;
				DisplayMessageBox(20);
			}break;
		}
		for (int i=0;i<8;i++) {
			CUTSCENE_FLAGS[i]=false;
		}
		CUTSCENE_TIMER=0;
	}

	void updateGame(){
		if (GAME_STATE!=FIN&&GAME_STATE!=THANKS) {
			frameCount++;
		}
		TIMER++;
		if (CURRENT_CUTSCENE!=cutscene::NONE) {
			CUTSCENE_TIMER++;
		}
		if (audioFade&&audioLevel>0) {
			audioLevel-=0.01;
			engine.SetOutputVolume(audioLevel);
			if (audioLevel<=0&&queueBGMPlayback) {
				queueBGMPlayback=false;
				fadeIn();
				playMusic(&SONG_EXPLORE);
			}
		} else
		if (!audioFade&&audioLevel<0.6&&!SOUND_IS_MUTED) {
			audioLevel+=0.01;
			engine.SetOutputVolume(audioLevel);
		}
		if (fade&&transparency<255) {
			transparency=std::clamp(transparency+FADE_SPD,0,255);
			if (transparency==255) {
				fadeOutCompleted();
			}
		} else
		if (!fade&&transparency>0) {
			transparency=std::clamp(transparency-FADE_SPD,0,255);
			if (transparency==0) {
				fadeInCompleted();
			}
		}
		if (messageBoxVisible) {
			if (frameCount%MESSAGE_SCROLL_WAIT_SPD==0) {
				if (messageBoxCursor<messageBoxRefText.length()) {
					advanceMessageBox();
				}
			}
		}
		if(frameCount%current_playerAnim->skip_frames==0) {
			current_playerAnim->frame=(current_playerAnim->frame+1)%current_playerAnim->frames.size();
		}
		for (auto anim:updateAnimationsList) {
			if (frameCount%anim->skip_frames==0) {
				anim->frame=(anim->frame+1)%anim->frames.size();
			}
		}

		if (!IN_BATTLE_ENCOUNTER) {
			bool zoneEffectActive=false;
			for (int i=0;i<ZONES.size();i++) {
				Zone*z = ZONES[i];
				if (PLAYER_COORDS[0]>=z->pos.x&&PLAYER_COORDS[0]<=z->pos.x+z->size.x&&
					PLAYER_COORDS[1]>=z->pos.y&&PLAYER_COORDS[1]<=z->pos.y+z->size.y) {
					if (ACTIVE_ZONE==nullptr) {
						applyPixelEffect(z->eff,{PLAYER_COORDS[0],PLAYER_COORDS[1]},0.9);
						ACTIVE_ZONE=z;
					}
					zoneEffectActive=true;
				}
			}
			if (!zoneEffectActive&&ACTIVE_ZONE!=nullptr) {
				clearPixelEffect();
				ACTIVE_ZONE=nullptr;
			}
		}
		
		if (playerCanMove()) {
			bool animationchanged=false;
			bool positionModified=false;
			if (RightHeld()) {
				if (GAME_STATE!=IN_SPACE&&MAP[(int)PLAYER_COORDS[1]][(int)std::clamp(PLAYER_COORDS[0]+MOVE_SPD,0.1,(double)MAP_WIDTH)]!=4) {
					PLAYER_COORDS[0]=std::clamp(PLAYER_COORDS[0]+MOVE_SPD,0.1,(double)MAP_WIDTH);
					positionModified=true;
				}
				//ConsoleClear();
				//cout<<"("<<PLAYER_COORDS[0]<<","<<PLAYER_COORDS[1]<<+")";
				if (!animationchanged&&&current_playerAnim!=&playerAnimWalkRight) {
					changeAnimation(playerAnimWalkRight);
					animationchanged=true;
				}
			}
			if (LeftHeld()) {
				if (GAME_STATE!=IN_SPACE&&MAP[(int)PLAYER_COORDS[1]][(int)std::clamp(PLAYER_COORDS[0]-MOVE_SPD,0.1,(double)MAP_WIDTH)]!=4) {
					PLAYER_COORDS[0]=std::clamp(PLAYER_COORDS[0]-MOVE_SPD,0.1,(double)MAP_WIDTH);
					positionModified=true;
				}
				//ConsoleClear();
				//cout<<"("<<PLAYER_COORDS[0]<<","<<PLAYER_COORDS[1]<<+")";
				if (!animationchanged&&&current_playerAnim!=&playerAnimWalkLeft) {
					changeAnimation(playerAnimWalkLeft);
					animationchanged=true;
				}
			}
			if (UpHeld()) {
				if (GAME_STATE!=IN_SPACE&&MAP[(int)std::clamp(PLAYER_COORDS[1]-MOVE_SPD,0.1,(double)MAP_HEIGHT)][(int)PLAYER_COORDS[0]]!=4) {
					PLAYER_COORDS[1]=std::clamp(PLAYER_COORDS[1]-MOVE_SPD,0.1,(double)MAP_HEIGHT);
					positionModified=true;
				}
				//ConsoleClear();
				//cout<<"("<<PLAYER_COORDS[0]<<","<<PLAYER_COORDS[1]<<+")";
				if (!animationchanged&&&current_playerAnim!=&playerAnimWalkUp) {
					changeAnimation(playerAnimWalkUp);
					animationchanged=true;
				}
				if (!messageBoxVisible&&BATTLE_STATE==battle::PLAYER_SELECTION) {
					HIDE_CARDS=true;
				}
			} else {
				HIDE_CARDS=false;
			}
			if (DownHeld()) {
				if (GAME_STATE!=IN_SPACE&&MAP[(int)std::clamp(PLAYER_COORDS[1]+MOVE_SPD,0.1,(double)MAP_HEIGHT)][(int)PLAYER_COORDS[0]]!=4) {
					PLAYER_COORDS[1]=std::clamp(PLAYER_COORDS[1]+MOVE_SPD,0.1,(double)MAP_HEIGHT);
					positionModified=true;
				}
				//ConsoleClear();
				//cout<<"("<<PLAYER_COORDS[0]<<","<<PLAYER_COORDS[1]<<+")";
				if (!animationchanged&&&current_playerAnim!=&playerAnimWalkDown) {
					changeAnimation(playerAnimWalkDown);
					animationchanged=true;
				}
			}
			if (positionModified) {
				playerMoved();
			}
			for (int i=0;i<OBJECTS.size();i++) {
				Object*obj = OBJECTS[i];
				if (obj->name.compare("HAILSTORM_NODE")==0&&collidesWithPlayer(obj)) {
					int amountGained=rand()%2+5;
					HAILSTORM->playerOwnCount+=amountGained;
					std::cout<<"Increased HAILSTORM power inventory count by "<<amountGained<<".\n";
					COLLECTED_ITEMS.push_back({(int)obj->x,(int)obj->y});
					OBJECTS.erase(OBJECTS.begin()+i--);
					delete obj;
					performCropUpdate(48);
					displayPowerInfo(HAILSTORM);
				} else
				if (obj->name.compare("HURRICANE_NODE")==0&&collidesWithPlayer(obj)) {
					int amountGained=rand()%2+5;
					HURRICANE->playerOwnCount+=amountGained;
					std::cout<<"Increased HURRICANE power inventory count by "<<amountGained<<".\n";
					COLLECTED_ITEMS.push_back({(int)obj->x,(int)obj->y});
					OBJECTS.erase(OBJECTS.begin()+i--);
					delete obj;
					performCropUpdate(48);
					displayPowerInfo(HURRICANE);
				} else
				if (obj->name.compare("METEORSHOWER_NODE")==0&&collidesWithPlayer(obj)) {
					int amountGained=rand()%2+5;
					METEOR_RAIN->playerOwnCount+=amountGained;
					std::cout<<"Increased METEORSHOWER power inventory count by "<<amountGained<<".\n";
					COLLECTED_ITEMS.push_back({(int)obj->x,(int)obj->y});
					OBJECTS.erase(OBJECTS.begin()+i--);
					delete obj;
					performCropUpdate(48);
					displayPowerInfo(METEOR_RAIN);
				} else
				if (obj->name.compare("METEORSTORM_NODE")==0&&collidesWithPlayer(obj)) {
					int amountGained=rand()%2+5;
					METEOR_STORM->playerOwnCount+=amountGained;
					std::cout<<"Increased METEORSTORM power inventory count by "<<amountGained<<".\n";
					COLLECTED_ITEMS.push_back({(int)obj->x,(int)obj->y});
					OBJECTS.erase(OBJECTS.begin()+i--);
					delete obj;
					performCropUpdate(48);
					displayPowerInfo(METEOR_STORM);
				} else
				if (obj->name.compare("SNOWSTORM_NODE")==0&&collidesWithPlayer(obj)) {
					int amountGained=rand()%2+5;
					SNOWSTORM->playerOwnCount+=amountGained;
					std::cout<<"Increased SNOWSTORM power inventory count by "<<amountGained<<".\n";
					COLLECTED_ITEMS.push_back({(int)obj->x,(int)obj->y});
					OBJECTS.erase(OBJECTS.begin()+i--);
					delete obj;
					performCropUpdate(48);
					displayPowerInfo(SNOWSTORM);
				} else
				if (obj->name.compare("PETALSTORM_NODE")==0&&collidesWithPlayer(obj)) {
					int amountGained=rand()%2+5;
					PETAL_STORM->playerOwnCount+=amountGained;
					std::cout<<"Increased PETALSTORM power inventory count by "<<amountGained<<".\n";
					COLLECTED_ITEMS.push_back({(int)obj->x,(int)obj->y});
					OBJECTS.erase(OBJECTS.begin()+i--);
					delete obj;
					performCropUpdate(48);
					displayPowerInfo(PETAL_STORM);
				} else
				if (obj->name.compare("SUNNYDAY_NODE")==0&&collidesWithPlayer(obj)) {
					int amountGained=rand()%2+5;
					SUNNY_DAY->playerOwnCount+=amountGained;
					std::cout<<"Increased SUNNYDAY power inventory count by "<<amountGained<<".\n";
					COLLECTED_ITEMS.push_back({(int)obj->x,(int)obj->y});
					OBJECTS.erase(OBJECTS.begin()+i--);
					delete obj;
					performCropUpdate(48);
					displayPowerInfo(SUNNY_DAY);
				} else
				if (obj->name.compare("FLASHFLOOD_NODE")==0&&collidesWithPlayer(obj)) {
					int amountGained=rand()%2+5;
					FLASH_FLOOD->playerOwnCount+=amountGained;
					std::cout<<"Increased FLASHFLOOD power inventory count by "<<amountGained<<".\n";
					COLLECTED_ITEMS.push_back({(int)obj->x,(int)obj->y});
					OBJECTS.erase(OBJECTS.begin()+i--);
					delete obj;
					performCropUpdate(48);
					displayPowerInfo(FLASH_FLOOD);
				} else
				if (obj->name.compare("FIRESTORM_NODE")==0&&collidesWithPlayer(obj)) {
					int amountGained=rand()%2+5;
					FIRESTORM->playerOwnCount+=amountGained;
					std::cout<<"Increased FIRESTORM power inventory count by "<<amountGained<<".\n";
					COLLECTED_ITEMS.push_back({(int)obj->x,(int)obj->y});
					OBJECTS.erase(OBJECTS.begin()+i--);
					delete obj;
					performCropUpdate(48);
					displayPowerInfo(FIRESTORM);
				} else
				if (obj->name.compare("SOLARFLARE_NODE")==0&&collidesWithPlayer(obj)) {
					int amountGained=rand()%2+5;
					SOLAR_FLARE->playerOwnCount+=amountGained;
					std::cout<<"Increased SOLARFLARE power inventory count by "<<amountGained<<".\n";
					COLLECTED_ITEMS.push_back({(int)obj->x,(int)obj->y});
					OBJECTS.erase(OBJECTS.begin()+i--);
					delete obj;
					performCropUpdate(48);
					displayPowerInfo(SOLAR_FLARE);
				} else
				if (obj->name.compare("SILICON_PIECE")==0&&collidesWithPlayer(obj)) {
					GAME_STATE=COLLECTED_SILICON;
					DisplayMessageBox(14);
					PlaySound(&SOUND_SELECT);
					COLLECTED_ITEMS.push_back({(int)obj->x,(int)obj->y});
					OBJECTS.erase(OBJECTS.begin()+i--);
					delete obj;
				} else
				if (obj->name.compare("EXIT")==0&&collidesWithPlayer(obj)) {
					fadeOut();
					PlayCutscene(cutscene::TRANSITION_CUTSCENE);
				} else
				if (obj->name.compare("DOME")==0&&collidesWithPlayer(obj)) {
					fadeOut();
					PlayCutscene(cutscene::TRANSITION_CUTSCENE_2);
				}
			}
		}

		if (!IN_BATTLE_ENCOUNTER&&playerCanMove()) {
			for (int i=0;i<ENCOUNTERS.size();i++) {
				Encounter enc = ENCOUNTERS[i];
				if (enc.map==CURRENT_MAP&&collidesWithPlayer(enc)) {
					for (int i=0;i<WEATHER_POWER_COUNT;i++) {
						if (WEATHER_POWERS[i]->name.compare("Snack")==0||WEATHER_POWERS[i]->name.compare("Meal")==0) {
							WEATHER_POWERS[i]->playerOwnCount=foodCount;
						}
					}
					bool containsFinalBoss=false;
					for (int i=0;i<enc.entities.size();i++) {
						if (enc.entities[i]->name.compare("A.A")==0) {
							containsFinalBoss=true;
							break;
						}
					}
					if (containsFinalBoss) {
						playMusic(&SONG_FINALBATTLE);
					} else {
						playMusic(&SONG_BATTLE);
					}
					availablePowers.erase(availablePowers.begin(),availablePowers.end());
					IN_BATTLE_ENCOUNTER=true;
					CURRENT_ENCOUNTER=enc;
					CURRENT_ENCOUNTER_IND=i;
					BATTLE_ENTRY_TIMER=0;
					BATTLE_STATE=battle::WAITING_FOR_CAMERA;
					for (int i=0;i<WEATHER_POWER_COUNT;i++) {
						if (WEATHER_POWERS[i]->playerOwnCount>0&&WEATHER_POWERS[i]!=ENERGY_BALL) {
							availablePowers.push_back(WEATHER_POWERS[i]);
						}
					}
					BATTLE_PLAYER_COORDS={PLAYER_COORDS[0],PLAYER_COORDS[1]};
					BATTLE_CARD_SELECTION_IND=0;
					BATTLE_CARD_SELECTION=availablePowers[BATTLE_CARD_SELECTION_IND];
					PLAYER_SELECTED_TARGET=-1;
					LOCKED_IN_DELAY=0;
				}
			}
		}

		for (int i=0;i<BATTLE_DISPLAY_NUMBERS.size();i++) {
			DisplayNumber*numb=BATTLE_DISPLAY_NUMBERS[i];
			if (frameCount-numb->frame>60) {
				BATTLE_DISPLAY_NUMBERS.erase(BATTLE_DISPLAY_NUMBERS.begin()+i--);
				delete numb;
			} else {
				numb->y-=0.01;
				numb->alpha-=4;
			}
		}

		if (IN_BATTLE_ENCOUNTER&&!messageBoxVisible) {
			BATTLE_ENTRY_TIMER++;
			switch (BATTLE_STATE) {
				case battle::WAITING_FOR_CAMERA: {
					if (BATTLE_ENTRY_TIMER>45) {
						int TARGET_COORDS_X=CURRENT_ENCOUNTER.x+WIDTH/32/2;
						int TARGET_COORDS_Y=CURRENT_ENCOUNTER.y+HEIGHT/32/2;
						if (PLAYER_COORDS[0]==TARGET_COORDS_X&&PLAYER_COORDS[1]==TARGET_COORDS_Y
							&&BATTLE_PLAYER_COORDS.x==CURRENT_ENCOUNTER.x+CURRENT_ENCOUNTER.playerX&&BATTLE_PLAYER_COORDS.y==CURRENT_ENCOUNTER.y+CURRENT_ENCOUNTER.playerY) {
							if (LOCKED_IN_DELAY>30) {
								if (!GAME_FLAGS[gameflag::FIRST_ENCOUNTER_X_X]&&GAME_FLAGS[gameflag::SLEEP]) {
									GAME_FLAGS[gameflag::FIRST_ENCOUNTER_X_X]=true;
									DisplayMessageBox(22);
									break;
								}
								//std::cout<<"Battle State is "<<BATTLE_STATE<<" (2)\n";
								BATTLE_STATE = battle::PLAYER_SELECTION;
								clearPixelEffect();
								EFFECT_TIMER = 0;
								bool hasPower=false;
								for (int i=0;i<availablePowers.size();i++) {
									if (availablePowers[i]->playerOwnCount>0&&availablePowers[i]!=CONSUME_SNACK&&availablePowers[i]!=CONSUME_MEAL) {
										hasPower=true;
										break;
									}
								}
								if (!hasPower) {
									availablePowers.push_back(ENERGY_BALL);
								}
							} else {
								LOCKED_IN_DELAY++;
							}
						}
						if (PLAYER_COORDS[0]!=TARGET_COORDS_X) {
							if (PLAYER_COORDS[0]<TARGET_COORDS_X) {
								PLAYER_COORDS[0]+=BATTLE_CAMERA_SCROLL_SPD;
								if (PLAYER_COORDS[0]>TARGET_COORDS_X) {
									PLAYER_COORDS[0]=TARGET_COORDS_X;
								}
							} else {
								PLAYER_COORDS[0]-=BATTLE_CAMERA_SCROLL_SPD;
								if (PLAYER_COORDS[0]<TARGET_COORDS_X) {
									PLAYER_COORDS[0]=TARGET_COORDS_X;
								}
							}
						}
						if (PLAYER_COORDS[1]!=TARGET_COORDS_Y) {
							if (PLAYER_COORDS[1]<TARGET_COORDS_Y) {
								PLAYER_COORDS[1]+=BATTLE_CAMERA_SCROLL_SPD;
								if (PLAYER_COORDS[1]>TARGET_COORDS_Y) {
									PLAYER_COORDS[1]=TARGET_COORDS_Y;
								}
							} else {
								PLAYER_COORDS[1]-=BATTLE_CAMERA_SCROLL_SPD;
								if (PLAYER_COORDS[1]<TARGET_COORDS_Y) {
									PLAYER_COORDS[1]=TARGET_COORDS_Y;
								}
							}
						}
						if (BATTLE_PLAYER_COORDS.x!=CURRENT_ENCOUNTER.x+CURRENT_ENCOUNTER.playerX) {
							if (BATTLE_PLAYER_COORDS.x<CURRENT_ENCOUNTER.x+CURRENT_ENCOUNTER.playerX) {
								BATTLE_PLAYER_COORDS.x+=BATTLE_CAMERA_SCROLL_SPD;
								if (BATTLE_PLAYER_COORDS.x>CURRENT_ENCOUNTER.x+CURRENT_ENCOUNTER.playerX) {
									BATTLE_PLAYER_COORDS.x=CURRENT_ENCOUNTER.x+CURRENT_ENCOUNTER.playerX;
								}
							} else {
								BATTLE_PLAYER_COORDS.x-=BATTLE_CAMERA_SCROLL_SPD;
								if (BATTLE_PLAYER_COORDS.x<CURRENT_ENCOUNTER.x+CURRENT_ENCOUNTER.playerX) {
									BATTLE_PLAYER_COORDS.x=CURRENT_ENCOUNTER.x+CURRENT_ENCOUNTER.playerX;
								}
							}
						}
						if (BATTLE_PLAYER_COORDS.y!=CURRENT_ENCOUNTER.y+CURRENT_ENCOUNTER.playerY) {
							if (BATTLE_PLAYER_COORDS.y<CURRENT_ENCOUNTER.y+CURRENT_ENCOUNTER.playerY) {
								BATTLE_PLAYER_COORDS.y+=BATTLE_CAMERA_SCROLL_SPD;
								if (BATTLE_PLAYER_COORDS.y>CURRENT_ENCOUNTER.y+CURRENT_ENCOUNTER.playerY) {
									BATTLE_PLAYER_COORDS.y=CURRENT_ENCOUNTER.y+CURRENT_ENCOUNTER.playerY;
								}
							} else {
								BATTLE_PLAYER_COORDS.y-=BATTLE_CAMERA_SCROLL_SPD;
								if (BATTLE_PLAYER_COORDS.y<CURRENT_ENCOUNTER.y+CURRENT_ENCOUNTER.playerY) {
									BATTLE_PLAYER_COORDS.y=CURRENT_ENCOUNTER.y+CURRENT_ENCOUNTER.playerY;
								}
							}
						}
					}
				}break;
				case battle::PLAYER_SELECTION:{
					if (PETRIFY_TURNS>0) {
						setupBattleTurns();
						if (FOOD_REGEN_TURNS>0) {
							FOOD_REGEN_TURNS--;	
							PLAYER_HP=std::clamp(PLAYER_HP+(int)((PLAYER_MAXHP*0.33)*(BATTLE_DROUGHT_ACTIVE?0.5:1)),0,PLAYER_MAXHP);
							CreateDisplayNumber((int)(-PLAYER_MAXHP*0.33),CURRENT_ENCOUNTER.x+CURRENT_ENCOUNTER.playerX,CURRENT_ENCOUNTER.y+CURRENT_ENCOUNTER.playerY,frameCount);
						}
					}
					if (!GAME_FLAGS[gameflag::TUTORIAL1_X_X]&&GAME_FLAGS[gameflag::ANALYSIS_X_X]) {
						GAME_FLAGS[gameflag::TUTORIAL1_X_X]=true;
						bool hasPetalStorm=false;
						for (int i=0;i<availablePowers.size();i++) {
							if (availablePowers[i]->name.compare("Petal Storm")==0) {
								BATTLE_CARD_SELECTION=availablePowers[i];
								BATTLE_CARD_SELECTION_IND=i;
								hasPetalStorm=true;
								break;
							}
						}
						if (hasPetalStorm) {
							DisplayMessageBox(24);
						}
					} else
					if (!GAME_FLAGS[gameflag::TUTORIAL2_X_X]&&GAME_FLAGS[gameflag::TUTORIAL1_X_X]) {
						GAME_FLAGS[gameflag::TUTORIAL2_X_X]=true;
						bool hasPetalStorm=false;
						for (int i=0;i<availablePowers.size();i++) {
							if (availablePowers[i]->name.compare("Petal Storm")==0) {
								BATTLE_CARD_SELECTION=availablePowers[i];
								BATTLE_CARD_SELECTION_IND=i;
								hasPetalStorm=true;
								break;
							}
						}
						if (hasPetalStorm) {
							DisplayMessageBox(25);
						}
					}
				}break;
				case battle::WAIT_TURN_ANIMATION:{
					if (EFFECT_TIMER==0) {
						PIXEL_EFFECT_TRANSPARENCY=0;
						if (BATTLE_CURRENT_TURN_ENTITY==-1) {
							if (PLAYER_SELECTED_TARGET==-2) {
								applyPixelEffect(BATTLE_CARD_SELECTION,{CURRENT_ENCOUNTER.x+CURRENT_ENCOUNTER.playerX,CURRENT_ENCOUNTER.y+CURRENT_ENCOUNTER.playerY},0);
							} else {
								applyPixelEffect(BATTLE_CARD_SELECTION,{(float)(CURRENT_ENCOUNTER.x+CURRENT_ENCOUNTER.entities[PLAYER_SELECTED_TARGET]->x),(float)(CURRENT_ENCOUNTER.y+CURRENT_ENCOUNTER.entities[PLAYER_SELECTED_TARGET]->y)},0);
							}
						} else {
							if (CURRENT_ENCOUNTER.entities[BATTLE_CURRENT_TURN_ENTITY]->selectedMove->damage<0) {
								applyPixelEffect(CURRENT_ENCOUNTER.entities[BATTLE_CURRENT_TURN_ENTITY]->selectedMove,{(float)CURRENT_ENCOUNTER.x+CURRENT_ENCOUNTER.entities[BATTLE_CURRENT_TURN_ENTITY]->x,(float)CURRENT_ENCOUNTER.y+CURRENT_ENCOUNTER.entities[BATTLE_CURRENT_TURN_ENTITY]->y},0);
							} else {
								applyPixelEffect(CURRENT_ENCOUNTER.entities[BATTLE_CURRENT_TURN_ENTITY]->selectedMove,{CURRENT_ENCOUNTER.x+CURRENT_ENCOUNTER.playerX,CURRENT_ENCOUNTER.y+CURRENT_ENCOUNTER.playerY},0);
							}
						}
					}
					EFFECT_TIMER++;
					WEATHER_POWER*ref;
					if (BATTLE_CURRENT_TURN_ENTITY==-1) {
						ref=BATTLE_CARD_SELECTION;
					} else {
						ref=CURRENT_ENCOUNTER.entities[BATTLE_CURRENT_TURN_ENTITY]->selectedMove;
					}
					if (EFFECT_TIMER==1) {
						PlaySound(ref->sound,false,ref->soundSpd);
					}
					PIXEL_EFFECT_TRANSPARENCY=(0.5*cos(((M_PI/(30.0*(ref->effectTime/120.0)))/2)*EFFECT_TIMER-M_PI)+0.5);
					if (EFFECT_TIMER==30&&ref->name.compare("Petal Storm")==0) {
						int healPower=30;
						int healRoll=(int)(healPower+rand()%ref->damageRoll*sign(healPower)*(BATTLE_DROUGHT_ACTIVE?0.5:1));
						if (BATTLE_CURRENT_TURN_ENTITY==-1) {
							effectRadius({(int)BATTLE_PLAYER_COORDS.x,(int)BATTLE_PLAYER_COORDS.y},ref,-healRoll,true);
						} else {
							effectRadius({(int)(CURRENT_ENCOUNTER.entities[BATTLE_CURRENT_TURN_ENTITY]->x+CURRENT_ENCOUNTER.x+CURRENT_ENCOUNTER.entities[BATTLE_CURRENT_TURN_ENTITY]->spr->sprite->width*CURRENT_ENCOUNTER.entities[BATTLE_CURRENT_TURN_ENTITY]->sprScale.x/2/32),(int)(CURRENT_ENCOUNTER.entities[BATTLE_CURRENT_TURN_ENTITY]->y+CURRENT_ENCOUNTER.y+CURRENT_ENCOUNTER.entities[BATTLE_CURRENT_TURN_ENTITY]->spr->sprite->height*CURRENT_ENCOUNTER.entities[BATTLE_CURRENT_TURN_ENTITY]->sprScale.y/2/32)},ref,-healRoll,false);
						}
					}
					if (EFFECT_TIMER==30&&(
						ref->name.compare("Seed of Life")==0||
						ref->name.compare("Seed Storm")==0)) {
						if (BATTLE_CURRENT_TURN_ENTITY==-1) {
							effectRadius({(int)BATTLE_PLAYER_COORDS.x,(int)BATTLE_PLAYER_COORDS.y},ref,true);
						} else {
							effectRadius({(int)(CURRENT_ENCOUNTER.entities[BATTLE_CURRENT_TURN_ENTITY]->x+CURRENT_ENCOUNTER.x+CURRENT_ENCOUNTER.entities[BATTLE_CURRENT_TURN_ENTITY]->spr->sprite->width*CURRENT_ENCOUNTER.entities[BATTLE_CURRENT_TURN_ENTITY]->sprScale.x/2/32),(int)(CURRENT_ENCOUNTER.entities[BATTLE_CURRENT_TURN_ENTITY]->y+CURRENT_ENCOUNTER.y+CURRENT_ENCOUNTER.entities[BATTLE_CURRENT_TURN_ENTITY]->spr->sprite->height*CURRENT_ENCOUNTER.entities[BATTLE_CURRENT_TURN_ENTITY]->sprScale.y/2/32)},ref,false);
						}
					} else 
					if (EFFECT_TIMER==ref->effectTime-10){
						if (!isBuffMove(ref)) {
							if (BATTLE_CURRENT_TURN_ENTITY==-1) {
								if (FOOD_REGEN_TURNS>0) {
									FOOD_REGEN_TURNS--;	
									PLAYER_HP=std::clamp(PLAYER_HP+(int)((PLAYER_MAXHP*0.33)*(BATTLE_DROUGHT_ACTIVE?0.5:1)),0,PLAYER_MAXHP);
									CreateDisplayNumber((int)(-PLAYER_MAXHP*0.33),CURRENT_ENCOUNTER.x+CURRENT_ENCOUNTER.playerX,CURRENT_ENCOUNTER.y+CURRENT_ENCOUNTER.playerY,frameCount);
									
								}
								if (PLAYER_SELECTED_TARGET==-2) {
									effectRadius({(int)(CURRENT_ENCOUNTER.x+CURRENT_ENCOUNTER.playerX),(int)(CURRENT_ENCOUNTER.y+CURRENT_ENCOUNTER.playerY)},ref,true);
								} else {
									effectRadius({(int)(CURRENT_ENCOUNTER.entities[PLAYER_SELECTED_TARGET]->x+CURRENT_ENCOUNTER.x+CURRENT_ENCOUNTER.entities[PLAYER_SELECTED_TARGET]->spr->sprite->width*CURRENT_ENCOUNTER.entities[PLAYER_SELECTED_TARGET]->sprScale.x/2/32),(int)(CURRENT_ENCOUNTER.entities[PLAYER_SELECTED_TARGET]->y+CURRENT_ENCOUNTER.y+CURRENT_ENCOUNTER.entities[PLAYER_SELECTED_TARGET]->spr->sprite->height*CURRENT_ENCOUNTER.entities[PLAYER_SELECTED_TARGET]->sprScale.y/2/32)},ref,true);							}
							} else {
								effectRadius({(int)(CURRENT_ENCOUNTER.x+CURRENT_ENCOUNTER.playerX),(int)(CURRENT_ENCOUNTER.y+CURRENT_ENCOUNTER.playerY)},ref,false);
								if (ref==LIGHT_STORM&&CURRENT_ENCOUNTER.entities[BATTLE_CURRENT_TURN_ENTITY]->name.compare("X_X")==0) {
									CURRENT_ENCOUNTER.entities[BATTLE_CURRENT_TURN_ENTITY]->spr=X_X_UNCHARGED_DECAL;
								} else
								if (ref==SEED_PELLET&&CURRENT_ENCOUNTER.entities[BATTLE_CURRENT_TURN_ENTITY]->name.compare("X_X")==0) {
									CURRENT_ENCOUNTER.entities[BATTLE_CURRENT_TURN_ENTITY]->spr=X_X_DECAL;
								}
							}
						}
					} else 
					if (EFFECT_TIMER==ref->effectTime/2){
						addSeeds(ref->seedProduction);
						for (int i=0;i<TREES.size();i++) {
							if (ref->treeSeedChance>range(0,1)) {
								addSeeds(1);
							}
						}
						int newSeedCount=SEEDS.size()*ref->seedScatter;
						while (SEEDS.size()<newSeedCount) {
							addSeeds(1);
						}
					}
					if (ref->appliesSlow) {
						if (BATTLE_CURRENT_TURN_ENTITY==-1) {
							for (int i=0;i<CURRENT_ENCOUNTER.entities.size();i++) {
								CURRENT_ENCOUNTER.entities[i]->speed=std::clamp(CURRENT_ENCOUNTER.entities[i]->speed-1,-1,1);
							}
						} else {
								//Not implemented for enemies.
						}
					}
					if (ref->appliesSpeed) {
						if (BATTLE_CURRENT_TURN_ENTITY==-1) {
							//Not implemented for the player.
						} else {
							CURRENT_ENCOUNTER.entities[BATTLE_CURRENT_TURN_ENTITY]->speed=std::clamp(CURRENT_ENCOUNTER.entities[BATTLE_CURRENT_TURN_ENTITY]->speed+1,-1,1);
						}
					}
					if (ref->appliesHide) {
						if (BATTLE_CURRENT_TURN_ENTITY==-1) {
							//Not implemented for the player.
						} else {
							CURRENT_ENCOUNTER.entities[BATTLE_CURRENT_TURN_ENTITY]->hidden=true;
						}
					}
					if (EFFECT_TIMER>ref->effectTime) {
						EFFECT_TIMER=0;
						clearPixelEffect();
						if (ref->growSeeds&&SEEDS.size()>0) {
							BATTLE_REASON_CODE=reasoncode::SEED_GROWTH;
							BATTLE_STATE=battle::WAIT_TURN_ANIMATION2;
						} else 
						if (ref->burnTrees&&TREES.size()>0&&ref->treeBurnChance>range(0,1)){
							BATTLE_REASON_CODE=reasoncode::TREE_BURN;
							BATTLE_STATE=battle::WAIT_TURN_ANIMATION2;
						} else  
						if (ref==DROUGHT&&!BATTLE_DROUGHT_ACTIVE) {
							BATTLE_REASON_CODE=reasoncode::DROUGHT;
							BATTLE_STATE=battle::WAIT_TURN_ANIMATION2;
							BATTLE_DROUGHT_ACTIVE=true;
						} else  
						if (ref->appliesRadioactive&&(SEEDS.size()>0||TREES.size()>0)) {
							BATTLE_REASON_CODE=reasoncode::RADIOACTIVE;
							BATTLE_STATE=battle::WAIT_TURN_ANIMATION2;
						} else 
						if (ref->appliesPetrification) {
							BATTLE_REASON_CODE=reasoncode::PETRIFIED;
							BATTLE_STATE=battle::WAIT_TURN_ANIMATION2;
						} else {
							BATTLE_REASON_CODE=-1;
							BATTLE_STATE=battle::DAMAGE_RESOLUTION;
						}
					}
				}break;
				case battle::WAIT_TURN_ANIMATION2:{
					EFFECT_TIMER++;
					PIXEL_EFFECT_TRANSPARENCY=0.5*cos(M_PI/60*EFFECT_TIMER-M_PI)+0.5;
					int WAIT_THRESHOLD=BATTLE_REASON_CODE==reasoncode::DROUGHT?120:60;
					if (EFFECT_TIMER==10) {
						switch (BATTLE_REASON_CODE) {
							case reasoncode::SEED_GROWTH:{
								applyPixelEffect(SEED_STORM,BATTLE_PLAYER_COORDS,0);
							}break;
							case reasoncode::TREE_BURN:{
								applyPixelEffect(FIRESTORM_EFF,BATTLE_PLAYER_COORDS,0);
							}break;
							case reasoncode::DROUGHT:{
								applyPixelEffect(DROUGHT_EFF,BATTLE_PLAYER_COORDS,0);
							}break;
						}
					}
					if (EFFECT_TIMER==WAIT_THRESHOLD) {
						switch (BATTLE_REASON_CODE) {
							case reasoncode::SEED_GROWTH:{
								int healAmt=(int)(-10*(int)(SEEDS.size())*(BATTLE_DROUGHT_ACTIVE?0.5:1));
								std::cout<<"Heal Amt: "<<healAmt<<"\n";
								CreateDisplayNumber(healAmt,BATTLE_PLAYER_COORDS.x,BATTLE_PLAYER_COORDS.y,frameCount);
								PLAYER_HP=std::clamp(PLAYER_HP+healAmt*-1,0,PLAYER_MAXHP);
								addTrees(SEEDS.size());
								clearSeeds();
							}break;
							case reasoncode::TREE_BURN:{
								int dmgAmt=10*TREES.size();
								for (int i=0;i<CURRENT_ENCOUNTER.entities.size();i++) {
									if (CURRENT_ENCOUNTER.entities[i]->hp>0) {
										CreateDisplayNumber(dmgAmt,CURRENT_ENCOUNTER.x+CURRENT_ENCOUNTER.entities[i]->x+CURRENT_ENCOUNTER.entities[i]->spr->sprite->width*CURRENT_ENCOUNTER.entities[i]->sprScale.x/2/32,CURRENT_ENCOUNTER.y+CURRENT_ENCOUNTER.entities[i]->y+CURRENT_ENCOUNTER.entities[i]->spr->sprite->height*CURRENT_ENCOUNTER.entities[i]->sprScale.y/2/32,frameCount);
										CURRENT_ENCOUNTER.entities[i]->hp=std::clamp(CURRENT_ENCOUNTER.entities[i]->hp-dmgAmt,0,CURRENT_ENCOUNTER.entities[i]->maxhp);
									}
								}
								clearTrees();
							}break;
							case reasoncode::RADIOACTIVE:{
								if (BATTLE_CURRENT_TURN_ENTITY==-1) {
									//Not implemented for the player.
								} else {
									SEEDS.clear();
									TREES.clear();
								}
							}break;
							case reasoncode::PETRIFIED:{
								if (BATTLE_CURRENT_TURN_ENTITY==-1) {
									//Not implemented for the player.
								} else {
									PETRIFY_TURNS=3;
								}
							}break;
						}
					}
					if (EFFECT_TIMER==WAIT_THRESHOLD*2) {
						EFFECT_TIMER=0;
						clearPixelEffect();
						BATTLE_REASON_CODE=-1;
						BATTLE_STATE=battle::DAMAGE_RESOLUTION;
					}
				}break;
				case battle::DAMAGE_RESOLUTION:{
					EFFECT_TIMER++;
					if (EFFECT_TIMER>60) {
						if (BATTLE_CURRENT_TURN_ENTITY==-1) {
							BATTLE_DROUGHT_ACTIVE=false;
							std::cout<<"Drought turned off.\n";
							for (int i=0;i<WEATHER_POWER_COUNT;i++) {
								if (WEATHER_POWERS[i]->name.compare("Snack")==0||WEATHER_POWERS[i]->name.compare("Meal")==0) {
									WEATHER_POWERS[i]->playerOwnCount=foodCount;
								}
							}
						} else {
							if (CURRENT_ENCOUNTER.entities[BATTLE_CURRENT_TURN_ENTITY]->lastSlowVal==CURRENT_ENCOUNTER.entities[BATTLE_CURRENT_TURN_ENTITY]->speed&&
								CURRENT_ENCOUNTER.entities[BATTLE_CURRENT_TURN_ENTITY]->speed!=0&&rand()%3==0) {
								CURRENT_ENCOUNTER.entities[BATTLE_CURRENT_TURN_ENTITY]->speed=0;
							}
							if (CURRENT_ENCOUNTER.entities[BATTLE_CURRENT_TURN_ENTITY]->lastHiddenVal==CURRENT_ENCOUNTER.entities[BATTLE_CURRENT_TURN_ENTITY]->hidden&&
								CURRENT_ENCOUNTER.entities[BATTLE_CURRENT_TURN_ENTITY]->hidden&&rand()%3==0) {
								CURRENT_ENCOUNTER.entities[BATTLE_CURRENT_TURN_ENTITY]->hidden=0;
							}
							CURRENT_ENCOUNTER.entities[BATTLE_CURRENT_TURN_ENTITY]->lastSlowVal=CURRENT_ENCOUNTER.entities[BATTLE_CURRENT_TURN_ENTITY]->speed;
							CURRENT_ENCOUNTER.entities[BATTLE_CURRENT_TURN_ENTITY]->lastHiddenVal=CURRENT_ENCOUNTER.entities[BATTLE_CURRENT_TURN_ENTITY]->hidden;
							if (CURRENT_ENCOUNTER.entities[BATTLE_CURRENT_TURN_ENTITY]->name.compare("A.A")==0) {
								if (CURRENT_ENCOUNTER.entities[BATTLE_CURRENT_TURN_ENTITY]->fixedTurnOrderInd==0) {
									CURRENT_ENCOUNTER.entities[BATTLE_CURRENT_TURN_ENTITY]->spr=A_A_DECAL;
								} else {
									CURRENT_ENCOUNTER.entities[BATTLE_CURRENT_TURN_ENTITY]->spr=A_A_RECHARGE_DECAL;
								}
							}
						}
						if (turnOrder.empty()) {
							bool allDead=true;
							for (auto&ent:CURRENT_ENCOUNTER.entities) {
								if (ent->hp>0) {
									allDead=false;
									break;
								}
							}
							if (allDead) {
								bool snakeFight=false;
								bool YYFight=false;
								bool AAFight=false;
								queueBGMPlayback=true;
								if (!GAME_FLAGS[gameflag::LITTLESNAKES_KILLED]) {
									for (int i=0;i<CURRENT_ENCOUNTER.entities.size();i++) {
										if (CURRENT_ENCOUNTER.entities[i]->name.compare("Sidewinder")==0) {
											snakeFight=true;
											break;
										}
									}
									if (snakeFight) {
										GAME_FLAGS[gameflag::LITTLESNAKES_KILLED]=true;
										CURRENT_ENCOUNTER.entities[1]->hp=CURRENT_ENCOUNTER.entities[1]->maxhp;
										CURRENT_ENCOUNTER.entities[2]->hp=CURRENT_ENCOUNTER.entities[2]->maxhp;
										CURRENT_ENCOUNTER.entities[4]->hp=CURRENT_ENCOUNTER.entities[4]->maxhp;
										DisplayMessageBox(48);
										break;
									}
								}
								if (!GAME_FLAGS[gameflag::DEFEATED_Y_Y]) {
									for (int i=0;i<CURRENT_ENCOUNTER.entities.size();i++) {
										if (CURRENT_ENCOUNTER.entities[i]->name.compare("Y.Y")==0) {
											YYFight=true;
											break;
										}
									}
									if (YYFight) {
										GAME_FLAGS[gameflag::DEFEATED_Y_Y]=true;
										queueBGMPlayback=false;
										stopMusic();
										fadeIn();
										DisplayMessageBox(58);
										StartCutscene(cutscene::A_A_ENCOUNTER);
										SHIP_COLOR=Pixel(89, 55, 36,255);
									}
								}
								if (!GAME_FLAGS[gameflag::DEFEATED_A_A]) {
									for (int i=0;i<CURRENT_ENCOUNTER.entities.size();i++) {
										if (CURRENT_ENCOUNTER.entities[i]->name.compare("A.A")==0) {
											AAFight=true;
											break;
										}
									}
									if (AAFight) {
										GAME_FLAGS[gameflag::DEFEATED_A_A]=true;
										StartCutscene(cutscene::ROVER_SAVE);
										queueBGMPlayback=false;
										playMusic(&SONG_MAIN);
										for (int i=0;i<150;i++) {
											starpixels[i]->spd={range(-2,0),range(-0.1,0)};
										}
									}
								}
								audioFadeOut();
								IN_BATTLE_ENCOUNTER=false;
								ENCOUNTERS.erase(ENCOUNTERS.begin()+CURRENT_ENCOUNTER_IND);
								resetBattleState();
								PLAYER_COORDS[0]=BATTLE_PLAYER_COORDS.x;
								PLAYER_COORDS[1]=BATTLE_PLAYER_COORDS.y;	
								performCropUpdate(48);
								if (FOOD_REGEN_TURNS>0) {
									FOOD_REGEN_TURNS=0;
									foodCount++;
								}
							} else {
								if (PLAYER_HP<=0) {
									GameOver();
									break;
								}
								if (!GAME_FLAGS[gameflag::ANALYSIS_X_X]&&GAME_FLAGS[gameflag::FIRST_ENCOUNTER_X_X]) {
									GAME_FLAGS[gameflag::ANALYSIS_X_X]=true;
									DisplayMessageBox(23);
									break;
								}
							//std::cout<<"Battle State is "<<BATTLE_STATE<<" (3)\n";
								BATTLE_STATE=battle::PLAYER_SELECTION;
								PLAYER_SELECTED_TARGET=-1;
								PETRIFY_TURNS=std::clamp(PETRIFY_TURNS-1,0,3);
								bool hasPower=false;
								for (int i=0;i<availablePowers.size();i++) {
									if (availablePowers[i]->playerOwnCount>0&&availablePowers[i]!=CONSUME_SNACK&&availablePowers[i]!=CONSUME_MEAL) {
										hasPower=true;
										break;
									}
								}
								if (!hasPower) {
									availablePowers.push_back(ENERGY_BALL);
								}
							}
						} else {
							BATTLE_CURRENT_TURN_ENTITY=turnOrder.front();
							turnOrder.pop();
							if (BATTLE_CURRENT_TURN_ENTITY==-1) {
								if (PLAYER_HP<=0) {
									GameOver();
									break;
								}
							} else {
								if (CURRENT_ENCOUNTER.entities[BATTLE_CURRENT_TURN_ENTITY]->hp<=0) {
									break;
								}
							}
							BATTLE_STATE=battle::WAIT_TURN_ANIMATION;
							PIXEL_EFFECT_TRANSPARENCY=0;
							EFFECT_TIMER=0;
						}
					}
				}break;
			}
		}

		switch (CURRENT_CUTSCENE) {
			case cutscene::CUTSCENE_4:{
				if (!messageBoxVisible) {
					if (!CUTSCENE_FLAGS[0]) {
						CUTSCENE_OBJS[0]=CreateObject(GetMapFileCoords(8,17),PLAYER_DECAL,{64,0},{32,32},true);
						CUTSCENE_FLAGS[0]=true;
						DisplayMessageBox(0);
					} else
					if (!CUTSCENE_FLAGS[1]) {
						CUTSCENE_FLAGS[1]=true;
						DisplayMessageBox(1);
					} else
					{
						PlayCutscene(cutscene::PAN_OVER_TO_CROPS);
					}
				}
			}break;
			case cutscene::PAN_OVER_TO_CROPS:{
				if (CUTSCENE_OBJS[0]->y>4) {
					CUTSCENE_OBJS[0]->y-=MOVE_SPD;
				} else
				if (CUTSCENE_OBJS[0]->x>13) {
					CUTSCENE_OBJS[0]->spos={32,0};
					CUTSCENE_OBJS[0]->flipped=true;
					CUTSCENE_OBJS[0]->x-=MOVE_SPD;
				}
				if (MoveCameraTowardsPoint(GetMapFileCoords(7,11))) {
					PlayCutscene(cutscene::RAINING_IN_DOME);
					PlaySound(&SOUND_WEATHERLIGHT);
				}
			}break;
			case cutscene::RAINING_IN_DOME:{
				if (CUTSCENE_TIMER%60==0) {
					performCropUpdate(3);
				}
				if (CUTSCENE_TIMER>=60*6) {
					PIXEL_EFFECT_TRANSPARENCY=(60-(CUTSCENE_TIMER-60*6))/60;
				}
				if (CUTSCENE_TIMER>=60*7) {
					PlayCutscene(cutscene::AFTER_RAIN);
				}
			}break;
			case cutscene::AFTER_RAIN:{
				if (!messageBoxVisible) {
					if (!CUTSCENE_FLAGS[0]) {
						DisplayMessageBox(2);	
						CUTSCENE_FLAGS[0]=true;
					} else
					if (!CUTSCENE_FLAGS[1]) {
						DisplayMessageBox(3);	
						CUTSCENE_FLAGS[1]=true;
						foodMeterVisible=true;
					} else {
						PLAYER_COORDS[0]=13;
						PLAYER_COORDS[1]=4;
						changeAnimation(playerAnimLeft);
						EndCutscene();
					}
				}
			}break; 
			case cutscene::WALK_TO_COMPUTER:{
				if (!messageBoxVisible) {
					if (!CUTSCENE_FLAGS[0]) {
						DisplayMessageBox(11);	
						CUTSCENE_FLAGS[0]=true;
					} else
					if (!CUTSCENE_FLAGS[1]) {
						DisplayMessageBox(4);	
						CUTSCENE_FLAGS[1]=true;
					} else {
						vi2d targetPos = GetMapFileCoords(19,4);
						if (CUTSCENE_OBJS[0]->x<19) {
							CUTSCENE_OBJS[0]->x+=MOVE_SPD;
							if (CUTSCENE_OBJS[0]->x>19) {
								CUTSCENE_OBJS[0]->x=19;
							}
						}
						if (CUTSCENE_OBJS[0]->y<4) {
							CUTSCENE_OBJS[0]->y+=MOVE_SPD;
							if (CUTSCENE_OBJS[0]->y>4) {
								CUTSCENE_OBJS[0]->y=4;
							}
						}
						if (CUTSCENE_OBJS[0]->y>4) {
							CUTSCENE_OBJS[0]->y-=MOVE_SPD;
							if (CUTSCENE_OBJS[0]->y<4) {
								CUTSCENE_OBJS[0]->y=4;
							}
						}
						if (MoveCameraTowardsPoint({19,4})) {
							if (!CUTSCENE_FLAGS[2]) {
								DisplayMessageBox(5);
								CUTSCENE_FLAGS[2]=true;
							} else {
								PlayCutscene(cutscene::INPUT_USERNAME);
							}
						}
					}
				}
			}break; 
			case cutscene::GO_OUTSIDE:{
				if (!messageBoxVisible) {
					if (!CUTSCENE_FLAGS[0]) {
						CUTSCENE_FLAGS[0]=true;
						DisplayMessageBox(12);
					} else
					if (!CUTSCENE_FLAGS[1]) {
						CUTSCENE_FLAGS[1]=true;
						DisplayMessageBox(6);
					} else {
						PLAYER_COORDS[0]=19;
						PLAYER_COORDS[1]=4;
						EndCutscene();
					}
				}
			}break;
			case cutscene::WALK_TO_ROVER:{
				if (MoveObjectTowardsPoint({33,36},CUTSCENE_OBJS[0],HORZ_FIRST)) {
					CUTSCENE_OBJS[0]->spos={64,0};
				}
				if (MoveCameraTowardsPoint({33,36})) {
					if (!CUTSCENE_FLAGS[0]) {
						CUTSCENE_FLAGS[0]=true;
						CUTSCENE_TIMER=0;
					}
					if (CUTSCENE_TIMER>60) {
						if (!messageBoxVisible) {
							if (!CUTSCENE_FLAGS[3]) {
								CUTSCENE_FLAGS[3]=true;
								DisplayMessageBox(13);
							} else
							if (!CUTSCENE_FLAGS[1]) {
								CUTSCENE_FLAGS[1]=true;
								DisplayMessageBox(7);
							} else
							if (!CUTSCENE_FLAGS[2]) {
								CUTSCENE_FLAGS[2]=true;
								DisplayMessageBox(8);
							} else {
								PlayCutscene(cutscene::DISPLAY_VOLCANIC_AREA);
								fadeOut();
							}
						}
					}
				}
			}break;
			case cutscene::DISPLAY_VOLCANIC_AREA:{
				if (CUTSCENE_FLAGS[0]&&!messageBoxVisible) {
					if (!CUTSCENE_FLAGS[1]) {
						CUTSCENE_FLAGS[1]=true;
						DisplayMessageBox(9);
					} else {
						fadeOut();
						PlayCutscene(cutscene::END_VOLCANIC_AREA_CUTSCENE);
					}
				}
			}break;
			case cutscene::REPAIR_ROVER_1:{
				if (!messageBoxVisible) {
					if (!CUTSCENE_FLAGS[0]) {
						CUTSCENE_FLAGS[0]=true;
						DisplayMessageBox(16);
					} else 
					if (!CUTSCENE_FLAGS[1]) {
						CUTSCENE_FLAGS[1]=true;
						CUTSCENE_TIMER=0;
					} else 
					if (!CUTSCENE_FLAGS[4]&&CUTSCENE_TIMER>60) {
						for (int i=0;i<OBJECTS.size();i++) {
							if (OBJECTS[i]->x==33&&OBJECTS[i]->y==35) {
								OBJECTS[i]->spr=ROVER_DECAL;
								CUTSCENE_FLAGS[4]=true;
								break;
							}
						}
					} else 
					if (!CUTSCENE_FLAGS[2]&&CUTSCENE_TIMER>120) {
						DisplayMessageBox(17);	
						CUTSCENE_FLAGS[2]=true;
					} else 
					if (!CUTSCENE_FLAGS[3]&&CUTSCENE_TIMER>120) {
						fadeOut();
						CUTSCENE_FLAGS[3]=true;
					}
				}
			}break;
			case cutscene::GET_SOME_REST:{
				if (!messageBoxVisible) {
					if (!CUTSCENE_FLAGS[0]) {
						CUTSCENE_FLAGS[0]=true;
						DisplayMessageBox(19);
						GAME_FLAGS[gameflag::REST_IN_DOME]=true;
					} else 
					{
						EndCutscene();
					}
				}
			}break;
			case cutscene::IN_BED:{
				if (!CUTSCENE_FLAGS[0]&&MoveCameraTowardsPoint({CUTSCENE_OBJS[0]->x,CUTSCENE_OBJS[0]->y})) {
					CUTSCENE_FLAGS[0]=true;
					CUTSCENE_TIMER=0;
				} else
				if (CUTSCENE_FLAGS[0]&&!CUTSCENE_FLAGS[1]&&CUTSCENE_TIMER>60*4) {
					StartCutscene(cutscene::SHAKE);
					stopMusic();
				}
			}break;
			case cutscene::SHAKE:{
				if (CUTSCENE_TIMER<60*4) {
					if (frameCount%4==0) {
						PLAYER_COORDS[1]+=0.25;
					} else 
					if (frameCount%4==1) {
						PLAYER_COORDS[1]+=0.25;
					} else 
					if (frameCount%4==2) {
						PLAYER_COORDS[1]-=0.25;
					} else 
					if (frameCount%4==3) {
						PLAYER_COORDS[1]-=0.25;
					}
				} else {
					StartCutscene(cutscene::INVESTIGATE_X_X);
				}
				if (frameCount%20==0) {
					PlaySound(&SOUND_SAW);
				}
			}break;
			case cutscene::INVESTIGATE_X_X:{
				if (!messageBoxVisible) {
					if (!CUTSCENE_FLAGS[0]) {
						CUTSCENE_FLAGS[0]=true;
						DisplayMessageBox(21);
					} else {
						PLAYER_COORDS[0]=CUTSCENE_OBJS[1]->x;
						PLAYER_COORDS[1]=CUTSCENE_OBJS[1]->y;
						EndCutscene();
						ENCOUNTERS.push_back(ENCOUNTER_X_X);
					}
				}
			}break;
			case cutscene::CHECK_COORDS:{
				if (!messageBoxVisible) {
					if (!CUTSCENE_FLAGS[0]) {
						CUTSCENE_FLAGS[0]=true;
						DisplayMessageBox(28);
					} else {
						EndCutscene();
					}
				}
			}break;
			case cutscene::SPAWN_WORMS:{
				if (CUTSCENE_TIMER<60) {
					if (frameCount%4==0) {
						PLAYER_COORDS[1]+=0.25;
					} else 
					if (frameCount%4==1) {
						PLAYER_COORDS[1]+=0.25;
					} else 
					if (frameCount%4==2) {
						PLAYER_COORDS[1]-=0.25;
					} else 
					if (frameCount%4==3) {
						PLAYER_COORDS[1]-=0.25;
					}
				} else {
					if (!messageBoxVisible) {
						if (!CUTSCENE_FLAGS[0]) {
							CUTSCENE_FLAGS[0]=true;
							CUTSCENE_OBJS[1]=CreateObject({PLAYER_COORDS[0]-2,PLAYER_COORDS[1]-3},SANDWORM_DECAL,true);
							CUTSCENE_OBJS[3]=CreateObject({PLAYER_COORDS[0]+3,PLAYER_COORDS[1]-1},SANDWORM_DECAL,true);
							DisplayMessageBox(30);
						} else {
							EndCutscene();
							ENCOUNTERS.push_back(ENCOUNTER_SANDWORM_1);
						}
					}
				}
			}break;
			case cutscene::CHECK_COORDS_2:{
				if (!messageBoxVisible) {
					if (!CUTSCENE_FLAGS[0]) {
						CUTSCENE_FLAGS[0]=true;
						DisplayMessageBox(32);
					} else 
					if (!CUTSCENE_FLAGS[1]) {
						CUTSCENE_FLAGS[1]=true;
						DisplayMessageBox(33);
					} else {
						PlayCutscene(cutscene::MAP_TRANSITION);
						fadeOut();
					}
				}
			}break;
			case cutscene::CHECK_COORDS_3:{
				if (!messageBoxVisible) {
					if (!CUTSCENE_FLAGS[0]) {
						CUTSCENE_FLAGS[0]=true;
						fadeOut();
					}
				}
			}break;
			case cutscene::STORY_REVIEW:{
				if (!messageBoxVisible) {
					if (!CUTSCENE_FLAGS[0]) {
						if (MoveCameraTowardsPoint({18,5})&&MoveObjectTowardsPoint({19,2.5},CUTSCENE_OBJS[0],BOTH)) {
							CUTSCENE_FLAGS[0]=true;
						}
					} else
					if (!CUTSCENE_FLAGS[1]) {
						DisplayMessageBox(38);
						CUTSCENE_FLAGS[1]=true;
					} else
					if (!CUTSCENE_FLAGS[2]) {
						CUTSCENE_OBJS[0]->anim=playerAnimLeft;
						DisplayMessageBox(39);
						CUTSCENE_FLAGS[2]=true;
					} else {
						fadeOut();
					}
				}
			}break;
			case cutscene::STORY_REVIEW2:{
				if (CUTSCENE_TIMER>120) {
					if (!messageBoxVisible) {
						if (!CUTSCENE_FLAGS[0]) {
							DisplayMessageBox(40);
							CUTSCENE_FLAGS[0]=true;
						} else
						if (!CUTSCENE_FLAGS[1]) {
							DisplayMessageBox(41);
							CUTSCENE_FLAGS[1]=true;
						} else
						if (!CUTSCENE_FLAGS[2]) {
							DisplayMessageBox(42);
							CUTSCENE_FLAGS[2]=true;
						} else
						if (!CUTSCENE_FLAGS[3]) {
							GAME_STATE=DISPLAY_BOOK2;
							DisplayMessageBox(43);
							CUTSCENE_FLAGS[3]=true;
						} else
						if (!CUTSCENE_FLAGS[4]) {
							DisplayMessageBox(44);
							CUTSCENE_FLAGS[4]=true;
						} else
						if (!CUTSCENE_FLAGS[5]) {
							fadeOut();
							CUTSCENE_FLAGS[5]=true;
						}
					}
				}
			}break;
			case cutscene::FINAL_REVIEW:{
				if (CUTSCENE_TIMER>60) {
					if (!messageBoxVisible) {
						if (!CUTSCENE_FLAGS[0]) {
							CUTSCENE_FLAGS[0]=true;
							DisplayMessageBox(45);
						} else {
							current_playerAnim=playerAnimLeft;
							EndCutscene();
						}
					}
				}
			}break;
			case cutscene::CHECK_COORDS_4:{
				if (!messageBoxVisible) {
					if (!CUTSCENE_FLAGS[0]) {
						CUTSCENE_FLAGS[0]=true;
						DisplayMessageBox(47);
					} else {
						EndCutscene();
					}
				}
			}break;
			case cutscene::PREPARE_SEQUENCE:{
				if (!messageBoxVisible) {
					if (!CUTSCENE_FLAGS[0]) {
						CUTSCENE_FLAGS[0]=true;
						fadeOut();
					}
				}
			}break;
			case cutscene::LAUNCHPAD_OPEN:{
				if (MoveCameraTowardsPoint({30,35})) {
					if (!CUTSCENE_FLAGS[0]) {
						PlaySound(&SOUND_SAW,false,0.3);
						CUTSCENE_FLAGS[0]=true;
						stopMusic();
					}
					if (frameCount%16==0) {
						PLAYER_COORDS[0]+=0.25;
					} else 
					if (frameCount%16==4) {
						PLAYER_COORDS[0]+=0.25;
					} else 
					if (frameCount%16==8) {
						PLAYER_COORDS[0]-=0.25;
					} else 
					if (frameCount%16==12) {
						PLAYER_COORDS[0]-=0.25;
					}
					if (CUTSCENE_TIMER>60*8) {
						PlayCutscene(cutscene::LAUNCHPAD_PREPARE);
						CUTSCENE_OBJS[1]=CreateObject({30,35},LAUNCHPAD_DECAL,true);
						CUTSCENE_OBJS[2]=CreateObject({30,35},LAUNCHPAD_HALF1_DECAL,true);
						CUTSCENE_OBJS[3]=CreateObject({30,35},LAUNCHPAD_HALF2_DECAL,true);
					}
				}
			}break;
			case cutscene::LAUNCHPAD_PREPARE:{
				bool result1,result2;
				result1=MoveObjectTowardsPoint({29.5,35.28},CUTSCENE_OBJS[2],BOTH,0.001);
				result2=MoveObjectTowardsPoint({30.5,34.78},CUTSCENE_OBJS[3],BOTH,0.001);
				if (result1&&result2&&CUTSCENE_TIMER>600) {
					if (!messageBoxVisible) {
						if (!CUTSCENE_FLAGS[0]) {
							CUTSCENE_FLAGS[0]=true;
							DisplayMessageBox(50);
						} else 
						if (!CUTSCENE_FLAGS[1]) {
							CUTSCENE_FLAGS[1]=true;
							DisplayMessageBox(51);
							TIMER=0;
						} else 
						if (!CUTSCENE_FLAGS[2]&&TIMER>60) {
							CUTSCENE_FLAGS[2]=true;
							DisplayMessageBox(52);
						} else {
							GAME_FLAGS[gameflag::ROCKET_LAUNCH_READY]=true;
							playMusic(&SONG_DOME);
							fadeOut();
						}
					}
				}
			}break;
			case cutscene::INTRUDERS_DETECTED:{
				if (!messageBoxVisible) {
					if (!CUTSCENE_FLAGS[0]) {
						CUTSCENE_FLAGS[0]=true;
						DisplayMessageBox(54);
					} else
					if (!CUTSCENE_FLAGS[1]) {
						CUTSCENE_FLAGS[1]=true;
						DisplayMessageBox(55);
					} else 
					if (!CUTSCENE_FLAGS[2]) {
						CUTSCENE_FLAGS[2]=true;
						CUTSCENE_OBJS[0]=CreateObject({999.5,999.25},PLAYER_DECAL,playerAnim,true);
					} else 
					if (!CUTSCENE_FLAGS[5]&&MoveObjectTowardsPoint({999.5,1002},CUTSCENE_OBJS[0],VERT_FIRST)) {
						CUTSCENE_FLAGS[5]=true;
					} else
					if (CUTSCENE_FLAGS[5]) {
						if (!CUTSCENE_FLAGS[3]) {
							DisplayMessageBox(56);
							CUTSCENE_FLAGS[3]=true;
						} else 
						if (!CUTSCENE_FLAGS[4]) {
							DisplayMessageBox(57);
							CUTSCENE_FLAGS[4]=true;
						} else {
							updatePlayerState();
							EndCutscene();
							ENCOUNTERS.push_back(ENCOUNTER_Y_Y);
						}
					}
				}
			}break;
			case cutscene::A_A_ENCOUNTER:{
				if (!messageBoxVisible) {
					if (!CUTSCENE_FLAGS[0]) {
						CUTSCENE_FLAGS[0]=true;
						DisplayMessageBox(59);
						CUTSCENE_TIMER=0;
					} else
					if (!CUTSCENE_FLAGS[1]&&CUTSCENE_TIMER>90) {
						CUTSCENE_FLAGS[1]=true;
						DisplayMessageBox(60);
					} else
					if (!CUTSCENE_FLAGS[2]) {
						CUTSCENE_FLAGS[2]=true;
						DisplayMessageBox(61);
					} else {
						PlayCutscene(cutscene::A_A_TRANSFORM);
					}
				}
			}break;
			case cutscene::A_A_TRANSFORM:{
				if (CUTSCENE_TIMER<60) {
					if (!CUTSCENE_FLAGS[2]) {
						CUTSCENE_FLAGS[2]=true;
						PlaySound(&SOUND_SAW,false,0.4);
					}
					//89, 55, 36
					SHIP_COLOR=Pixel(
						std::clamp(89+(0.5*cos(M_PI/60*CUTSCENE_TIMER-M_PI)+0.5)*255,0.0,255.0),
						std::clamp(55+(0.5*cos(M_PI/60*CUTSCENE_TIMER-M_PI)+0.5)*255,0.0,255.0),
						std::clamp(36+(0.5*cos(M_PI/60*CUTSCENE_TIMER-M_PI)+0.5)*255,0.0,255.0),
						255
					);
				} else 
				if (!CUTSCENE_FLAGS[0]) {
					CUTSCENE_FLAGS[0]=true;
					TIMER=0;
				} else 
				if (TIMER<240) {
					BOSS_SIZE={
						(float)std::clamp(TIMER/240.0+1,1.0,2.0),
						(float)std::clamp(TIMER/240.0+1,1.0,2.0)
					};
					SHIP_COLOR=Pixel(255,255,255,(1-(TIMER/240.0))*255);
				} else {
					if (!messageBoxVisible) {
						if (!CUTSCENE_FLAGS[1]) {
							CUTSCENE_FLAGS[1]=true;
							DisplayMessageBox(62);
							engine.StopAll();
							PlaySound(&SOUND_INTROFINAL);
						} else {
							updatePlayerState();
							EndCutscene();
							ENCOUNTERS.push_back(ENCOUNTER_A_A);
						}
					}
				}
			}break;
			case cutscene::ROVER_SAVE:{
				if (!CUTSCENE_FLAGS[0]) {
					CUTSCENE_OBJS[0]=CreateObject({PLAYER_COORDS[0],PLAYER_COORDS[1]},PLAYER_DECAL,playerAnimDown,true);
					CUTSCENE_OBJS[1]=CreateObject({PLAYER_COORDS[0]-5,PLAYER_COORDS[1]},ROVER_DECAL,true);
					CUTSCENE_FLAGS[0]=true;
				} else {
					if (!CUTSCENE_FLAGS[1]) {
						if (CUTSCENE_OBJS[1]->x<PLAYER_COORDS[0]-0.5) {
							CUTSCENE_OBJS[1]->x+=0.02;
						} else {
							CUTSCENE_FLAGS[1]=true;	
						}
					} else 
					if (!messageBoxVisible) {
						if (!CUTSCENE_FLAGS[2]) {
							CUTSCENE_FLAGS[2]=true;
							DisplayMessageBox(63);
						} else 
						if (!CUTSCENE_FLAGS[3]) {
							CUTSCENE_FLAGS[3]=true;
							DisplayMessageBox(64);
						} else 
						if (!CUTSCENE_FLAGS[4]) {
							CUTSCENE_FLAGS[4]=true;
							DisplayMessageBox(65);
						} else 
						if (!CUTSCENE_FLAGS[5]) {
							CUTSCENE_OBJS[0]->x=0;
							if (CUTSCENE_OBJS[1]->x<PLAYER_COORDS[0]+4) {
								CUTSCENE_OBJS[1]->x+=0.06;
							} else {
								CUTSCENE_FLAGS[5]=true;
							}
						} else 
						if (!CUTSCENE_FLAGS[6]) {
							CUTSCENE_FLAGS[6]=true;
							fadeOut();
						}
					}
				}
				CUTSCENE_OBJS[1]->y=cos(M_PI/90*CUTSCENE_TIMER-M_PI)*0.5+PLAYER_COORDS[1];
			}break;
		}

		if (GAME_STATE==CUTSCENE_3&&!SOUND_IS_PLAYING) {
			playMusic(&SONG_MAIN);
			SOUND_IS_PLAYING=true;
		}

		switch (GAME_STATE) {
			case CUTSCENE_1:
			case CUTSCENE_3:
			case CUTSCENE_4:
			case CUTSCENE_5:
			case CUTSCENE_6:
			case GAME_OVER_TERMINAL:{
				int soundFrequency=-1;
				if (GAME_STATE==CUTSCENE_1) {
					soundFrequency=160;
				} else
				if (GAME_STATE==CUTSCENE_4) {
					soundFrequency=120;
				} else
				if (GAME_STATE==CUTSCENE_5) {
					soundFrequency=80;
				} else
				if (GAME_STATE==CUTSCENE_6) {
					soundFrequency=70;
				}
				if (soundFrequency!=-1&&frameCount%soundFrequency==0) {
					PlaySound(&SOUND_SONAR);
				}
				if (GAME_STATE==CUTSCENE_3&&frameCount%4!=0) {break;}
				if (textInd<CONSOLE_REF_TEXT.length()) {
					char c = CONSOLE_REF_TEXT[textInd++];
					CUTSCENE_CONSOLE_TEXT+=c;
					if (c!='\n') {
						cursorX++;
					} else {
						cursorX=0;
					}
					if (GetTextSize(CUTSCENE_CONSOLE_TEXT).x>WIDTH-32) {
						int tempIndex=textInd;
						while (CUTSCENE_CONSOLE_TEXT[--tempIndex]!=' ') {
							CUTSCENE_CONSOLE_TEXT.erase(tempIndex);
							cursorX--;
						}
						CUTSCENE_CONSOLE_TEXT.erase(tempIndex++);
						CUTSCENE_CONSOLE_TEXT+='\n';
						cursorX=0;
						while (tempIndex<textInd) {
							CUTSCENE_CONSOLE_TEXT+=CONSOLE_REF_TEXT[tempIndex++];
							cursorX++;
						}
					}
				}
			}break;
			case CUTSCENE_2:{
				//FLAG 0 - When flipped on, camera stops.
				if (!CUTSCENE_FLAGS[0]) {
					PLAYER_COORDS[0]+=0.06;
				} else {
					if (CUTSCENE_TIMER>120) {
						fadeOut();
					}
				}
				if (PLAYER_COORDS[0]>38+(128/2/32)) {
					PLAYER_COORDS[0]=38+(128/2/32);
					CUTSCENE_FLAGS[0]=true;
					CUTSCENE_TIMER=0;
				}
			}break;
			case WAITING_FOR_CUTSCENE_3:{
				if (transparency>200) {
					GAME_STATE=CUTSCENE_3;
				}
			}break;
			case COLLECTED_SILICON:{
				if (!messageBoxVisible) {
					if (CURRENT_MAP==MAP_5) {
						if (!GAME_FLAGS[gameflag::COLLECTED_SILICON_4]) {
							GAME_FLAGS[gameflag::COLLECTED_SILICON_4]=true;
							updatePlayerState();
							GAME_STATE=GAMEWORLD;
						}
					} else
					if (CURRENT_MAP==MAP_4) {
						if (!GAME_FLAGS[gameflag::COLLECTED_SILICON_3]) {
							GAME_FLAGS[gameflag::COLLECTED_SILICON_3]=true;
							updatePlayerState();
							DisplayMessageBox(34);
							GAME_STATE=GAMEWORLD;
						}
					} else
					if (CURRENT_MAP==MAP_3) {
						if (!GAME_FLAGS[gameflag::COLLECTED_SILICON_2]) {
							GAME_FLAGS[gameflag::COLLECTED_SILICON_2]=true;
							updatePlayerState();
							DisplayMessageBox(29);
						} else {
							GAME_STATE=GAMEWORLD;
							StartCutscene(cutscene::SPAWN_WORMS);
							CUTSCENE_OBJS[0]=CreateObject({PLAYER_COORDS[0],PLAYER_COORDS[1]},PLAYER_DECAL,playerAnimDown,true);
						}
					} else {
						if (!GAME_FLAGS[gameflag::COLLECTED_SILICON_1]) {
							GAME_FLAGS[gameflag::COLLECTED_SILICON_1]=true;
							updatePlayerState();
							DisplayMessageBox(15);
						} else {
							GAME_STATE=GAMEWORLD;
						}
					}
				}
			}break;
			case LATER_THAT_NIGHTWAIT:{
				if (TIMER>180) {
					fadeOut();
					GAME_STATE=LATER_THAT_NIGHTFADEOUT;
				}
			}break;
			case IN_SPACE:{
				for (int i=0;i<150;i++) {
					starpixels[i]->pos+=starpixels[i]->spd;
					bool hitEdge=false;
					if (starpixels[i]->pos.x<STAR_POS.x) {
						starpixels[i]->pos.x=STAR_POS.x+STAR_SIZE.x;
						hitEdge=true;
					} else
					if (starpixels[i]->pos.x>STAR_POS.x+STAR_SIZE.x) {
						starpixels[i]->pos.x=STAR_POS.x;
						hitEdge=true;
					} else
					if (starpixels[i]->pos.y<STAR_POS.y) {
						starpixels[i]->pos.y=STAR_POS.y+STAR_SIZE.y;
						hitEdge=true;
					} else
					if (starpixels[i]->pos.y>STAR_POS.y+STAR_SIZE.y) {
						starpixels[i]->pos.y=STAR_POS.y;
						hitEdge=true;
					}
					if (hitEdge) {
						if (GAME_FLAGS[gameflag::DEFEATED_A_A]) {
							starpixels[i]->pos={WIDTH,range(0,HEIGHT)};
						} else {
							starpixels[i]->pos={WIDTH/2,HEIGHT/2-24};
						}
					}
				}
				if (TIMER>300) {
					if (!GAME_FLAGS[gameflag::INTRUDER_DETECTED]) {
						GAME_FLAGS[gameflag::INTRUDER_DETECTED]=true;
						DisplayMessageBox(53);
						stopMusic();
						PlayCutscene(cutscene::INTRUDERS_DETECTED);
					}
				}
			}break;
			case FIN:{
				CREDITS_SCROLLING_OFFSET.y-=0.15;
				if (CREDITS_SCROLLING_OFFSET.y*-1>GetTextSize(CREDITS_TEXT).y*2+24) {
					fadeOut();
				}
			}break;
		}

		if (PIXEL_EFFECT_TRANSPARENCY>0) {
			for (int i=0;i<PIXEL_LIMIT;i++) {
				pixels[i]->pos+=pixels[i]->spd;
				if (pixels[i]->pos.x<PIXEL_POS.x) {
					pixels[i]->pos.x=PIXEL_POS.x+PIXEL_SIZE.x;
				}
				if (pixels[i]->pos.x>PIXEL_POS.x+PIXEL_SIZE.x) {
					pixels[i]->pos.x=PIXEL_POS.x;
				}
				if (pixels[i]->pos.y<PIXEL_POS.y) {
					pixels[i]->pos.y=PIXEL_POS.y+PIXEL_SIZE.y;
				}
				if (pixels[i]->pos.y>PIXEL_POS.y+PIXEL_SIZE.y) {
					pixels[i]->pos.y=PIXEL_POS.y;
				}
				pixels[i]->a=pixels[i]->o_a*PIXEL_EFFECT_TRANSPARENCY;
			}
		}
	}

	void advanceMessageBox() {
		if (frameCount%4==0) {
			if (messageBoxSpeaker.compare("Y.Y")==0||messageBoxSpeaker.compare("A.A")==0) {
				PlaySound(&SOUND_ROBOTICNOISE);
			} else {
				PlaySound(&SOUND_MSG);
			}
		}
		char c = messageBoxRefText[messageBoxCursor++];
		printf("%c",c);
		if (c=='\n') {
			if (!firstNewline) {
				firstNewline=true;
				return;
			} else if (!secondNewline) {
				secondNewline=true;
				return;
			}
		}
		if (firstNewline&&!secondNewline) {
			messageBoxSpeaker+=c;
			return;
		}
		messageBoxText+=c;
		if (GetTextSizeProp(messageBoxText).x>WIDTH-16) {
			int tempIndex=messageBoxCursor;
			while (messageBoxText[--tempIndex]!=' ') {
				messageBoxText.erase(tempIndex);
			}
			messageBoxText.erase(tempIndex++);
			messageBoxText+='\n';
			while (tempIndex<messageBoxCursor) {
				messageBoxText+=messageBoxRefText[tempIndex++];
			}
		}
	}

	//Triggers when the player has successfully moved.
	void playerMoved(){
		if (!GAME_FLAGS[gameflag::TUTORIAL_WALKED_OFF_FARM]&&foodCount>3&&PLAYER_COORDS[0]>12) {
			GAME_FLAGS[gameflag::TUTORIAL_WALKED_OFF_FARM]=true;
			PlayCutscene(cutscene::WALK_TO_COMPUTER);
			CUTSCENE_OBJS[0]=CreateObject({PLAYER_COORDS[0],PLAYER_COORDS[1]},PLAYER_DECAL,{32,0},{32,32},true);
		} else
		if (!GAME_FLAGS[gameflag::REPAIRED_ROVER_1]&&GAME_FLAGS[gameflag::COLLECTED_SILICON_1]&&PLAYER_COORDS[0]>=31&&PLAYER_COORDS[0]<=35&&PLAYER_COORDS[1]>=33&&PLAYER_COORDS[1]<=37) {
			GAME_FLAGS[gameflag::REPAIRED_ROVER_1]=true;
			PlayCutscene(cutscene::REPAIR_ROVER_1);
		} else
		if (!GAME_FLAGS[gameflag::NEXT_COORDS]&&GAME_FLAGS[gameflag::CHECK_ROVER]&&PLAYER_COORDS[0]>=31&&PLAYER_COORDS[0]<=35&&PLAYER_COORDS[1]>=33&&PLAYER_COORDS[1]<=37) {
			GAME_FLAGS[gameflag::NEXT_COORDS]=true;
			DisplayMessageBox(27);
			StartCutscene(cutscene::CHECK_COORDS);
			CUTSCENE_OBJS[0]=CreateObject({PLAYER_COORDS[0],PLAYER_COORDS[1]},PLAYER_DECAL,playerAnim,true);
		} else
		if (!GAME_FLAGS[gameflag::CHECK_ROVER_2]&&GAME_FLAGS[gameflag::COLLECTED_SILICON_2]&&PLAYER_COORDS[0]>=31&&PLAYER_COORDS[0]<=35&&PLAYER_COORDS[1]>=33&&PLAYER_COORDS[1]<=37) {
			GAME_FLAGS[gameflag::CHECK_ROVER_2]=true;
			DisplayMessageBox(31);
			StartCutscene(cutscene::CHECK_COORDS_2);
			CUTSCENE_OBJS[0]=CreateObject({PLAYER_COORDS[0],PLAYER_COORDS[1]},PLAYER_DECAL,playerAnim,true);
		} else
		if (!GAME_FLAGS[gameflag::CHECK_ROVER_3]&&GAME_FLAGS[gameflag::COLLECTED_SILICON_3]&&PLAYER_COORDS[0]>=31&&PLAYER_COORDS[0]<=35&&PLAYER_COORDS[1]>=33&&PLAYER_COORDS[1]<=37) {
			GAME_FLAGS[gameflag::CHECK_ROVER_3]=true;
			DisplayMessageBox(35);
			StartCutscene(cutscene::CHECK_COORDS_3);
			CUTSCENE_OBJS[0]=CreateObject({PLAYER_COORDS[0],PLAYER_COORDS[1]},PLAYER_DECAL,playerAnim,true);
		} else
		if (!GAME_FLAGS[gameflag::CHECK_ROVER_4]&&GAME_FLAGS[gameflag::NEXT_COORDS2]&&PLAYER_COORDS[0]>=31&&PLAYER_COORDS[0]<=35&&PLAYER_COORDS[1]>=33&&PLAYER_COORDS[1]<=37) {
			GAME_FLAGS[gameflag::CHECK_ROVER_4]=true;
			DisplayMessageBox(46);
			StartCutscene(cutscene::CHECK_COORDS_4);
			CUTSCENE_OBJS[0]=CreateObject({PLAYER_COORDS[0],PLAYER_COORDS[1]},PLAYER_DECAL,playerAnim,true);
		} else
		if (!GAME_FLAGS[gameflag::CHECK_ROVER_5]&&GAME_FLAGS[gameflag::COLLECTED_SILICON_4]&&PLAYER_COORDS[0]>=31&&PLAYER_COORDS[0]<=35&&PLAYER_COORDS[1]>=33&&PLAYER_COORDS[1]<=37) {
			GAME_FLAGS[gameflag::CHECK_ROVER_5]=true;
			DisplayMessageBox(49);
			StartCutscene(cutscene::PREPARE_SEQUENCE);
			CUTSCENE_OBJS[0]=CreateObject({PLAYER_COORDS[0],PLAYER_COORDS[1]},PLAYER_DECAL,playerAnim,true);
		} else
		if (!GAME_FLAGS[gameflag::BOARDED_ROCKET]&&GAME_FLAGS[gameflag::ROCKET_LAUNCH_READY]&&PLAYER_COORDS[0]>=30.5&&PLAYER_COORDS[0]<=31.5&&PLAYER_COORDS[1]>=35&&PLAYER_COORDS[1]<=36.5) {
			StartCutscene(cutscene::READY_TO_BOARD);
			ROCKET_BOARD_OPTION=false;
		}
		if(WALK_STEPS++>60&&!IN_BATTLE_ENCOUNTER) {
			PLAYER_HP=std::clamp(PLAYER_HP+1,0,PLAYER_MAXHP);
			updatePlayerState();
			WALK_STEPS=0;
		}
	}

	void drawGame(){
		switch (GAME_STATE) {
			case CUTSCENE_1:
			case CUTSCENE_4:
			case CUTSCENE_5:
			case CUTSCENE_6:
			case GAME_OVER_TERMINAL:{
				DrawStringDecal({16,16},CUTSCENE_CONSOLE_TEXT,GREEN,{1,1});
				if (textInd<CONSOLE_REF_TEXT.length()) {
					FillRectDecal({(float)(16+(cursorX)*8%(WIDTH-32)),(float)(8+GetTextSize(CUTSCENE_CONSOLE_TEXT).y+((cursorX==28)?8:0))},{4,8},GREEN);
				} else {
					FillRectDecal({(float)(16+(cursorX)*8%(WIDTH-32)),(float)(8+GetTextSize(CUTSCENE_CONSOLE_TEXT).y+((cursorX==28)?8:0))},{4,8},Pixel(0,255,0,(0.5*sin(frameCount*4/60.0)+0.5)*256));
				}
				GradientFillRectDecal({0,0},{WIDTH/2,HEIGHT/2},{20, 28, 22,ALPHA_SCREEN1},{20, 28, 22,ALPHA_SCREEN1},{20, 28, 22,ALPHA_SCREEN2},{20, 28, 22,ALPHA_SCREEN1});
				GradientFillRectDecal({WIDTH/2,0},{WIDTH/2,HEIGHT/2},{20, 28, 22,ALPHA_SCREEN1},{20, 28, 22,ALPHA_SCREEN2},{20, 28, 22,ALPHA_SCREEN1},{20, 28, 22,ALPHA_SCREEN1});
				GradientFillRectDecal({0,HEIGHT/2},{WIDTH/2,HEIGHT/2},{20, 28, 22,ALPHA_SCREEN1},{20, 28, 22,ALPHA_SCREEN1},{20, 28, 22,ALPHA_SCREEN1},{20, 28, 22,ALPHA_SCREEN2});
				GradientFillRectDecal({WIDTH/2,HEIGHT/2},{WIDTH/2,HEIGHT/2},{20, 28, 22,ALPHA_SCREEN2},{20, 28, 22,ALPHA_SCREEN1},{20, 28, 22,ALPHA_SCREEN1},{20, 28, 22,ALPHA_SCREEN1});
			}break;
			case CUTSCENE_2:
			case GAMEWORLD:
			case COLLECTED_SILICON:{
				DrawMap();
				DrawGameWorld();
				int meterYOffset=2;
				if (CURRENT_CUTSCENE==cutscene::NONE) {
					SetDrawTarget(nullptr);
					if (foodMeterVisible) {
						DrawStringDecal({(float)(WIDTH-36*0.4-GetTextSize(std::to_string(foodCount)).x*1-8),(float)(meterYOffset+1)},std::to_string(foodCount),BLACK,{1,1});
						DrawStringDecal({(float)(WIDTH-36*0.4-GetTextSize(std::to_string(foodCount)).x*1-7),(float)(meterYOffset+2)},std::to_string(foodCount),WHITE,{1,1});
						DrawDecal({(float)(WIDTH-52*0.4),(float)(meterYOffset)},FOOD_METER_DECAL,{0.4,0.4});
						meterYOffset+=(2+48*0.4);
					}
					if (oxygenMeterVisible) {
						DrawStringDecal({(float)(WIDTH-36*0.4-GetTextSize(std::to_string(oxygenQualityLevel)+"%").x*1-8),(float)(meterYOffset+1)},std::to_string(oxygenQualityLevel)+"%",BLACK,{1,1});
						DrawStringDecal({(float)(WIDTH-36*0.4-GetTextSize(std::to_string(oxygenQualityLevel)+"%").x*1-7),(float)(meterYOffset+2)},std::to_string(oxygenQualityLevel)+"%",WHITE,{1,1});
						DrawDecal({(float)(WIDTH-52*0.4),(float)(meterYOffset)},OXYGEN_METER_DECAL,{0.4,0.4});
						meterYOffset+=(2+48*0.4);
					}
					SetDrawTarget(2);
					if (IN_BATTLE_ENCOUNTER) {
						DrawPartialDecal({(float)((BATTLE_PLAYER_COORDS.x-PLAYER_COORDS[0])*32+WIDTH/2-16+(current_playerAnim->flipped?32:0)),(float)((BATTLE_PLAYER_COORDS.y-PLAYER_COORDS[1])*32+HEIGHT/2-16)},current_playerAnim->spr,current_playerAnim->getCurrentFrame(),{32,32},{(float)(current_playerAnim->flipped?-1:1),1});
					} else {
						DrawPartialDecal({(float)(WIDTH/2-16+(current_playerAnim->flipped?32:0)),(float)(HEIGHT/2-16)},current_playerAnim->spr,current_playerAnim->getCurrentFrame(),{32,32},{(float)(current_playerAnim->flipped?-1:1),1});
					}
					if (IN_BATTLE_ENCOUNTER&&BATTLE_ENTRY_TIMER<45) {
						DrawStringDecal({(float)((BATTLE_PLAYER_COORDS.x-PLAYER_COORDS[0])*32+WIDTH/2-16+(current_playerAnim->flipped?32:0)),(float)((BATTLE_PLAYER_COORDS.y-PLAYER_COORDS[1])*32+HEIGHT/2-16-sin(frameCount*12/60.0)*4-12)},"!!",RED);
					}
				}
			}break;
			case CUTSCENE_3:{
				DrawStringDecal({48,16},CUTSCENE_CONSOLE_TEXT,Pixel(100, 10, 255),{2,2});
				if (textInd<STORY_TEXT2.length()) {
					FillRectDecal({(float)(48+(cursorX)*16%(WIDTH-32)),(float)(GetTextSize(CUTSCENE_CONSOLE_TEXT).y*2+((cursorX==28)?16:0))},{4*2,8*2},Pixel(100, 10, 255));
				} else {
					FillRectDecal({(float)(48+(cursorX)*16%(WIDTH-32)),(float)(GetTextSize(CUTSCENE_CONSOLE_TEXT).y*2+((cursorX==28)?16:0))},{4*2,8*2},Pixel(100, 10, 255,(0.5*sin(frameCount*4/60.0)+0.5)*256));
				}
				for (int i=0;i<4;i++) {
					std::string tempStr = MENU_OPTIONS[i];
					if (MAIN_MENU_SELECTION==i) {
						tempStr="> "+tempStr+" <";
					}
					DrawStringDecal({(float)(WIDTH/2-GetTextSize(tempStr).x*2/2),(float)(HEIGHT/2-GetTextSize(tempStr).y*2+32*i)},tempStr,(MAIN_MENU_SELECTION==i)?MAGENTA:WHITE,{2,2});
				}
				GradientFillRectDecal({0,0},{WIDTH/2,HEIGHT/2},{100, 10, 255,ALPHA_SCREEN1},{100, 10, 255,ALPHA_SCREEN1},{100, 10, 255,ALPHA_SCREEN2},{100, 10, 255,ALPHA_SCREEN1});
				GradientFillRectDecal({WIDTH/2,0},{WIDTH/2,HEIGHT/2},{100, 10, 255,ALPHA_SCREEN1},{100, 10, 255,ALPHA_SCREEN2},{100, 10, 255,ALPHA_SCREEN1},{100, 10, 255,ALPHA_SCREEN1});
				GradientFillRectDecal({0,HEIGHT/2},{WIDTH/2,HEIGHT/2},{100, 10, 255,ALPHA_SCREEN1},{100, 10, 255,ALPHA_SCREEN1},{100, 10, 255,ALPHA_SCREEN1},{100, 10, 255,ALPHA_SCREEN2});
				GradientFillRectDecal({WIDTH/2,HEIGHT/2},{WIDTH/2,HEIGHT/2},{100, 10, 255,ALPHA_SCREEN2},{100, 10, 255,ALPHA_SCREEN1},{100, 10, 255,ALPHA_SCREEN1},{100, 10, 255,ALPHA_SCREEN1});
			}break;
			case LATER_THAT_NIGHTFADEIN:
			case LATER_THAT_NIGHTFADEOUT:
			case LATER_THAT_NIGHTWAIT:{
				DrawDecal({(float)(WIDTH/2-LATER_THAT_NIGHT_DECAL->sprite->width/2),(float)(HEIGHT/2-LATER_THAT_NIGHT_DECAL->sprite->height/2)},LATER_THAT_NIGHT_DECAL);
			}break;
			case DISPLAY_BOOK:{
				DrawDecal({(float)(WIDTH/2-BOOK_DECAL->sprite->width*2/2),(float)(HEIGHT/2-BOOK_DECAL->sprite->height*2/2)},BOOK_DECAL,{2,2});
			}break;
			case DISPLAY_BOOK2:{
				DrawDecal({(float)(WIDTH/2-BOOK2_DECAL->sprite->width*2/2),(float)(HEIGHT/2-BOOK2_DECAL->sprite->height*2/2)},BOOK2_DECAL,{2,2});
			}break;
			case IN_SPACE:{
				Clear(BLACK);
				DrawStars();
				if (!IN_BATTLE_ENCOUNTER) {
					if (CURRENT_CUTSCENE==cutscene::A_A_TRANSFORM) {
						DrawDecal({WIDTH/2-32*BOSS_SIZE.x,HEIGHT/2-32*BOSS_SIZE.y},A_A_DECAL,BOSS_SIZE,Pixel(SHIP_COLOR.r,SHIP_COLOR.g,SHIP_COLOR.b,255-SHIP_COLOR.a));
					}
					DrawDecal({WIDTH/2-32*BOSS_SIZE.x,HEIGHT/2-32*BOSS_SIZE.y},Y_Y_DECAL,BOSS_SIZE,SHIP_COLOR);
				}
				DrawGameWorld();
				if (IN_BATTLE_ENCOUNTER) {
					DrawPartialDecal({(float)((BATTLE_PLAYER_COORDS.x-PLAYER_COORDS[0])*32+WIDTH/2-16+(current_playerAnim->flipped?32:0)),(float)((BATTLE_PLAYER_COORDS.y-PLAYER_COORDS[1])*32+HEIGHT/2-16)},current_playerAnim->spr,current_playerAnim->getCurrentFrame(),{32,32},{(float)(current_playerAnim->flipped?-1:1),1});
				}
				if (IN_BATTLE_ENCOUNTER&&BATTLE_ENTRY_TIMER<45) {
					DrawStringDecal({(float)((BATTLE_PLAYER_COORDS.x-PLAYER_COORDS[0])*32+WIDTH/2-16+(current_playerAnim->flipped?32:0)),(float)((BATTLE_PLAYER_COORDS.y-PLAYER_COORDS[1])*32+HEIGHT/2-16-sin(frameCount*12/60.0)*4-12)},"!!",RED);
				}
			}break;
			case FIN:{

				DrawStringDecal({CREDITS_SCROLLING_OFFSET.x+WIDTH/2-GetTextSize(CREDITS_TEXT).x*2/2,CREDITS_SCROLLING_OFFSET.y},CREDITS_TEXT,WHITE,{2,2});

				GradientFillRectDecal({0,0},{WIDTH/2,HEIGHT/2},{100, 10, 255,ALPHA_SCREEN1},{100, 10, 255,ALPHA_SCREEN1},{100, 10, 255,ALPHA_SCREEN2},{100, 10, 255,ALPHA_SCREEN1});
				GradientFillRectDecal({WIDTH/2,0},{WIDTH/2,HEIGHT/2},{100, 10, 255,ALPHA_SCREEN1},{100, 10, 255,ALPHA_SCREEN2},{100, 10, 255,ALPHA_SCREEN1},{100, 10, 255,ALPHA_SCREEN1});
				GradientFillRectDecal({0,HEIGHT/2},{WIDTH/2,HEIGHT/2},{100, 10, 255,ALPHA_SCREEN1},{100, 10, 255,ALPHA_SCREEN1},{100, 10, 255,ALPHA_SCREEN1},{100, 10, 255,ALPHA_SCREEN2});
				GradientFillRectDecal({WIDTH/2,HEIGHT/2},{WIDTH/2,HEIGHT/2},{100, 10, 255,ALPHA_SCREEN2},{100, 10, 255,ALPHA_SCREEN1},{100, 10, 255,ALPHA_SCREEN1},{100, 10, 255,ALPHA_SCREEN1});
			}break;
			case THANKS:{
				DrawStringDecal({(float)(WIDTH/2-GetTextSize(THANKS_TEXT).x*3/2),(float)(HEIGHT/2-GetTextSize(THANKS_TEXT).y*3/2)},THANKS_TEXT,WHITE,{3,3});
				DrawStringDecal({0,(float)(HEIGHT-GetTextSize("Playtime").y)},"Playtime: ",WHITE,{1,1});
				int hours=frameCount/60/60/60;
				int minutes=(frameCount/60/60)%60;
				int seconds=(frameCount/60)%60;
				DrawStringDecal({0,(float)(HEIGHT-GetTextSize("Playtime: ").y)},"Playtime: ",WHITE,{1,1});
				DrawStringDecal({(float)(GetTextSize("Playtime: ").x),(float)(HEIGHT-GetTextSize("Playtime: ").y)},(hours<10)?"0"+std::to_string(hours)+":":std::to_string(hours)+":",WHITE,{1,1});
				DrawStringDecal({(float)(GetTextSize("Playtime: ").x+24*1),(float)(HEIGHT-GetTextSize("Playtime: ").y)},(minutes<10)?"0"+std::to_string(minutes)+":":std::to_string(minutes)+":",WHITE,{1,1});
				DrawStringDecal({(float)(GetTextSize("Playtime: ").x+24*2),(float)(HEIGHT-GetTextSize("Playtime: ").y)},(seconds<10)?"0"+std::to_string(seconds):std::to_string(seconds),WHITE,{1,1});

				GradientFillRectDecal({0,0},{WIDTH/2,HEIGHT/2},{100, 10, 255,ALPHA_SCREEN1},{100, 10, 255,ALPHA_SCREEN1},{100, 10, 255,ALPHA_SCREEN2},{100, 10, 255,ALPHA_SCREEN1});
				GradientFillRectDecal({WIDTH/2,0},{WIDTH/2,HEIGHT/2},{100, 10, 255,ALPHA_SCREEN1},{100, 10, 255,ALPHA_SCREEN2},{100, 10, 255,ALPHA_SCREEN1},{100, 10, 255,ALPHA_SCREEN1});
				GradientFillRectDecal({0,HEIGHT/2},{WIDTH/2,HEIGHT/2},{100, 10, 255,ALPHA_SCREEN1},{100, 10, 255,ALPHA_SCREEN1},{100, 10, 255,ALPHA_SCREEN1},{100, 10, 255,ALPHA_SCREEN2});
				GradientFillRectDecal({WIDTH/2,HEIGHT/2},{WIDTH/2,HEIGHT/2},{100, 10, 255,ALPHA_SCREEN2},{100, 10, 255,ALPHA_SCREEN1},{100, 10, 255,ALPHA_SCREEN1},{100, 10, 255,ALPHA_SCREEN1});
			}
		}
		switch (BATTLE_STATE) {
			case battle::PLAYER_SELECTION:{
				if (!HIDE_CARDS) {
					SetDrawTarget(nullptr);
					DrawCard(availablePowers[(BATTLE_CARD_SELECTION_IND+1)%availablePowers.size()],{WIDTH-96,32},{0.7,0.7},0.4);
					DrawCard(availablePowers[(BATTLE_CARD_SELECTION_IND-1<0)?availablePowers.size()-1:BATTLE_CARD_SELECTION_IND-1],{-96,32},{0.7,0.7},0.4);
					DrawCard(BATTLE_CARD_SELECTION);
				}
			}break;
			case battle::PLAYER_TARGET_SELECTION:{
				if (PLAYER_SELECTED_TARGET>=0) {
					DrawWrappedText({5,5},"Use "+BATTLE_CARD_SELECTION->name+" on "+CURRENT_ENCOUNTER.entities[PLAYER_SELECTED_TARGET]->name+" "+(char)('A'+PLAYER_SELECTED_TARGET),WIDTH-8,BLACK,{2,2});
					DrawWrappedText({4,4},"Use "+BATTLE_CARD_SELECTION->name+" on "+CURRENT_ENCOUNTER.entities[PLAYER_SELECTED_TARGET]->name+" "+(char)('A'+PLAYER_SELECTED_TARGET),WIDTH-8,WHITE,{2,2});
					DrawStringDecal({(float)(WIDTH-GetTextSize("<UP> Back").x-1),(float)(HEIGHT-GetTextSize("<UP> Back").y-1)},"<UP> Back",BLACK,{1,1});
					DrawStringDecal({(float)(WIDTH-GetTextSize("<UP> Back").x-2),(float)(HEIGHT-GetTextSize("<UP> Back").y-2)},"<UP> Back",GREEN,{1,1});
				}
			}break;
			case battle::WAIT_TURN_ANIMATION:{
				if (BATTLE_CURRENT_TURN_ENTITY==-1) {
					DrawWrappedText({4,4},PLAYER_NAME+" uses "+BATTLE_CARD_SELECTION->name,WIDTH-8,WHITE,{2,2});
				} else {
					DrawWrappedText({4,4},CURRENT_ENCOUNTER.entities[BATTLE_CURRENT_TURN_ENTITY]->name+" "+(char)('A'+BATTLE_CURRENT_TURN_ENTITY)+" uses "+CURRENT_ENCOUNTER.entities[BATTLE_CURRENT_TURN_ENTITY]->selectedMove->name,WIDTH-8,WHITE,{2,2});
				}
			}break;
			case battle::WAIT_TURN_ANIMATION2:{
				switch (BATTLE_REASON_CODE) {
					case reasoncode::SEED_GROWTH:{
						DrawWrappedText({5,5},"The seeds grow into lush trees!",WIDTH-8,DARK_GREEN,{2,2});
						DrawWrappedText({4,4},"The seeds grow into lush trees!",WIDTH-8,WHITE,{2,2});
					}break;
					case reasoncode::TREE_BURN:{
						DrawWrappedText({5,5},"The trees cause rampaging fires!",WIDTH-8,DARK_RED,{2,2});
						DrawWrappedText({4,4},"The trees cause rampaging fires!",WIDTH-8,WHITE,{2,2});
					}break;
					case reasoncode::DROUGHT:{
						DrawWrappedText({5,5},"A drought is active, causing healing to be reduced!",WIDTH-8,DARK_MAGENTA,{2,2});
						DrawWrappedText({4,4},"A drought is active, causing healing to be reduced!",WIDTH-8,WHITE,{2,2});
					}break;
					case reasoncode::PETRIFIED:{
						DrawWrappedText({5,5},PLAYER_NAME+" became Petrified!",WIDTH-8,DARK_RED,{2,2});
						DrawWrappedText({4,4},PLAYER_NAME+" became Petrified!",WIDTH-8,WHITE,{2,2});
					}break;
					case reasoncode::RADIOACTIVE:{
						DrawWrappedText({5,5},"The radioactive environment made all flora die.",WIDTH-8,DARK_MAGENTA,{2,2});
						DrawWrappedText({4,4},"The radioactive environment made all flora die.",WIDTH-8,WHITE,{2,2});
					}break;
				}
			}break;
		}
		if (IN_BATTLE_ENCOUNTER||(GAME_STATE==GAMEWORLD&&!CUTSCENE_ACTIVE&&PLAYER_HP!=PLAYER_MAXHP)) {
			DrawStringDecal({4+1,(float)(HEIGHT-10-GetTextSize("HP:").y+1)},"HP: "+std::to_string(PLAYER_HP),BLACK);
			DrawStringDecal({4,(float)(HEIGHT-10-GetTextSize("HP:").y)},"HP: "+std::to_string(PLAYER_HP));
			DrawHealthbar({2,HEIGHT-10},WIDTH/2,(float)PLAYER_HP/PLAYER_MAXHP,BLACK);
			DrawBuffs({(float)(48+GetTextSize(std::to_string(PLAYER_HP)).x),(float)(HEIGHT-26)});
			for (auto&numb:BATTLE_DISPLAY_NUMBERS) {
				std::string display=((numb->number>0)?"-"+std::to_string(numb->number):"+"+std::to_string(numb->number*-1));
				for (int x=-1;x<=1;x++) {
					for (int y=-1;y<=1;y++) {
						if (x!=0&&y!=0) {
							DrawStringDecal({(float)(((numb->x-PLAYER_COORDS[0])*32+WIDTH/2)-GetTextSize(display).x*1.3/2+x-8),(float)(((numb->y-PLAYER_COORDS[1])*32+HEIGHT/2)-12-GetTextSize(display).y*1.3/2+y)},display,(numb->number>0)?Pixel(255,0,0,numb->alpha):Pixel(0,255,0,numb->alpha),{1.3,1.3});
						}
 					}
				}
				DrawStringDecal({(float)(((numb->x-PLAYER_COORDS[0])*32+WIDTH/2)-GetTextSize(display).x*1.3/2-8),(float)(((numb->y-PLAYER_COORDS[1])*32+HEIGHT/2)-12-GetTextSize(display).y*1.3/2)},display,Pixel(255,255,255,numb->alpha),{1.3,1.3});
				//std::cout<<numb->x<<"/"<<numb->y<<" "<<(((numb->x-PLAYER_COORDS[0])*32+WIDTH/2)-GetTextSize(display).x/2)<<","<<(((numb->y-PLAYER_COORDS[1])*32+HEIGHT/2)-8-GetTextSize(display).y/2)<<": ("<<numb->alpha<<")"<<display<<"\n";
			}
		}
		if (PIXEL_EFFECT_TRANSPARENCY>0) {
			SetDrawTarget(1);
			for (int i=0;i<PIXEL_LIMIT;i++) {
				if (pixels[i]->size.x==1&&pixels[i]->size.y==1) {
					Draw(pixels[i]->pos,{(uint8_t)(pixels[i]->r),(uint8_t)(pixels[i]->g),(uint8_t)(pixels[i]->b),(uint8_t)(pixels[i]->a)});
				} else {
					FillRectDecal(pixels[i]->pos,pixels[i]->size,{(uint8_t)(pixels[i]->r),(uint8_t)(pixels[i]->g),(uint8_t)(pixels[i]->b),(uint8_t)(pixels[i]->a)});
				}
			}
			FillRectDecal({0,0},{WIDTH,HEIGHT},Pixel(ORIGINAL_FOREGROUND_EFFECT_COLOR.r,ORIGINAL_FOREGROUND_EFFECT_COLOR.g,ORIGINAL_FOREGROUND_EFFECT_COLOR.b,ORIGINAL_FOREGROUND_EFFECT_COLOR.a*PIXEL_EFFECT_TRANSPARENCY));
			SetDrawTarget(2);
		}
		if (USE_TOUCH_CONTROLS&&(CURRENT_CUTSCENE==cutscene::NONE||CURRENT_CUTSCENE==cutscene::INPUT_USERNAME||CURRENT_CUTSCENE==cutscene::READY_TO_BOARD)&&(GAME_STATE!=CUTSCENE_1&&GAME_STATE!=CUTSCENE_4&&GAME_STATE!=CUTSCENE_5&&GAME_STATE!=CUTSCENE_6&&GAME_STATE!=GAME_OVER_TERMINAL&&GAME_STATE!=FIN&&GAME_STATE!=THANKS)) {
			DrawDecal({0,HEIGHT-128},TOUCHSCREEN_DECAL,{1,1},Pixel(255,255,255,128));
		}
		if (messageBoxVisible) {
			SetDrawTarget(nullptr);
			DrawDialogBox({4,HEIGHT-60},{WIDTH/2,16},Pixel(18, 0, 33,180));
			DrawDialogBox({0,HEIGHT-48},{WIDTH,48},Pixel(18, 0, 33,180));
			DrawStringPropDecal({8,HEIGHT-42},messageBoxText);
			DrawStringPropDecal({8,HEIGHT-57},messageBoxSpeaker);
			if (messageBoxCursor==messageBoxRefText.length()) {
				DrawStringPropDecal({WIDTH-16-(float)sin(frameCount*8/60.0)*3,HEIGHT-8+(float)(cos(frameCount*6/60.0)*0.6)},"v",Pixel(173, 74, 255,(0.5*sin(frameCount*8/60.0)+0.5)*128+128),{(float)sin(frameCount*8/60.0),0.5});
			}
			SetDrawTarget(2);
		}
		switch (CURRENT_CUTSCENE) {
			case cutscene::NODE_COLLECT_CUTSCENE:{
				FillRectDecal({0,0},{WIDTH,HEIGHT},Pixel(0,0,0,128));
				DrawCard(CUTSCENE_DISPLAYED_CARD);
			}break;
			case cutscene::INPUT_USERNAME:{
				FillRectDecal({0,0},{WIDTH,HEIGHT},Pixel(0,0,0,128));
				DrawTerminal();
			}break;
			case cutscene::IN_BED:{
				FillRectDecal({0,0},{WIDTH,HEIGHT},Pixel(0,0,0,192));
			}break;
			case cutscene::LAUNCHPAD_OPEN:{
				DrawDecal({(float)((30-PLAYER_COORDS[0])*32+WIDTH/2),(float)((35-PLAYER_COORDS[1])*32+HEIGHT/2+(6-6*((CUTSCENE_TIMER/240.0))))},LAUNCHPAD_CLOSED_DECAL,{1,1},Pixel(255,255,255,(CUTSCENE_TIMER/480.0)*255));
			}break;
			case cutscene::LAUNCHPAD_PREPARE:{
				int bottomCutoff = (int)(std::clamp(CUTSCENE_TIMER,0,600)/600.0*64);
				DrawPartialDecal({(float)((30-PLAYER_COORDS[0])*32+WIDTH/2),(float)((36.5-PLAYER_COORDS[1])*32+HEIGHT/2-bottomCutoff)},{64,(float)bottomCutoff},Y_Y_DECAL,{0,0},{64,(float)bottomCutoff},Pixel(std::clamp(CUTSCENE_TIMER,0,600)/600.0*255,std::clamp(CUTSCENE_TIMER,0,600)/600.0*255,std::clamp(CUTSCENE_TIMER,0,600)/600.0*255,255));
			}break;
			case cutscene::READY_TO_BOARD:{
				FillRectDecal({0,0},{WIDTH,HEIGHT},Pixel(0,0,0,128));
				DrawStringDecal({8,HEIGHT/4},"ARE YOU PREPARED ?");
				DrawStringDecal({(float)8,(float)(HEIGHT/2+((ROCKET_BOARD_OPTION)?0:16))},">");
				DrawStringDecal({16,HEIGHT/2},"YES");
				DrawStringDecal({16,HEIGHT/2+16},"NO");

				GradientFillRectDecal({0,0},{WIDTH/2,HEIGHT/2},{20, 28, 22,ALPHA_SCREEN1},{20, 28, 22,ALPHA_SCREEN1},{20, 28, 22,ALPHA_SCREEN2},{20, 28, 22,ALPHA_SCREEN1});
				GradientFillRectDecal({WIDTH/2,0},{WIDTH/2,HEIGHT/2},{20, 28, 22,ALPHA_SCREEN1},{20, 28, 22,ALPHA_SCREEN2},{20, 28, 22,ALPHA_SCREEN1},{20, 28, 22,ALPHA_SCREEN1});
				GradientFillRectDecal({0,HEIGHT/2},{WIDTH/2,HEIGHT/2},{20, 28, 22,ALPHA_SCREEN1},{20, 28, 22,ALPHA_SCREEN1},{20, 28, 22,ALPHA_SCREEN1},{20, 28, 22,ALPHA_SCREEN2});
				GradientFillRectDecal({WIDTH/2,HEIGHT/2},{WIDTH/2,HEIGHT/2},{20, 28, 22,ALPHA_SCREEN2},{20, 28, 22,ALPHA_SCREEN1},{20, 28, 22,ALPHA_SCREEN1},{20, 28, 22,ALPHA_SCREEN1});
			}break;
		}
		FillRectDecal({0,0},{WIDTH,HEIGHT},Pixel(0,0,0,transparency));
		//FillRectDecal({WIDTH/2-2,HEIGHT/2-2},{4,4},WHITE);
	}

	void DrawMap() {
		for (int y=-1;y<=HEIGHT/32+1;y++) {
			for (int x=-1;x<=WIDTH/32+1;x++) {
				float xOffset=PLAYER_COORDS[0]-(WIDTH/32/2);
				float yOffset=PLAYER_COORDS[1]-(HEIGHT/32/2);
				srand((int)(yOffset+y)*MAP_WIDTH+(int)(xOffset+x));
				int tileX=0;
				int tileRot=0;
				while (tileX<3&&std::rand()%4==0) {
					tileX++;
				}
				while (tileRot<3&&std::rand()%8<5) {
					tileRot++;
				}
				if ((int)(xOffset+x)>=0&&(int)(xOffset+x)<MAP_WIDTH&&(int)(yOffset+y)>=0&&(int)(yOffset+y)<MAP_HEIGHT) {
					DrawPartialRotatedDecal({(float)((x-(PLAYER_COORDS[0]-(int)PLAYER_COORDS[0])+1)*32-16),(float)((y-(PLAYER_COORDS[1]-(int)PLAYER_COORDS[1])+1)*32)},TILES,tileRot*M_PI_2,{16,16},{(float)(tileX*32),0},{32,32},{1,1},TILE_COLORS[MAP[(int)(yOffset+y)][(int)(xOffset+x)]]);
				}
			}
		}
	}

	void DrawGameWorld() {
		for (auto&obj:OBJECTS) {
			if (obj->spr!=NULL) {
				if (obj->name.compare("PLANT")==0) {
					DrawPartialDecal({(float)((obj->x-PLAYER_COORDS[0])*32+WIDTH/2),(float)((obj->y-PLAYER_COORDS[1])*32+HEIGHT/2)},{32,32},obj->spr,{(float)(getPlantStatus(obj->x,obj->y)*32),0},{32,32});
				} else {
					if (obj->name.find("NODE")!=std::string::npos) {
						DrawDecal({(obj->x-PLAYER_COORDS[0])*32+WIDTH/2-32+18,(obj->y-PLAYER_COORDS[1])*32+HEIGHT/2-32+26},WEATHERNODE_EFFECT_DECAL);
						DrawPartialDecal({(float)((obj->x-PLAYER_COORDS[0])*32+WIDTH/2-16*(float)sin(frameCount*2/60.0)+16),(float)((obj->y-PLAYER_COORDS[1])*32+HEIGHT/2)},obj->anim->spr,{(float)(obj->anim->getCurrentFrame().x),(float)(obj->anim->getCurrentFrame().y)},{(float)(obj->anim->width),(float)(obj->anim->height)},{(float)sin(frameCount*2/60.0),1},Pixel((float)sin(frameCount*4/60.0)*55+200,(float)sin(frameCount*4/60.0)*55+200,(float)sin(frameCount*4/60.0+M_PI)+65*125,255));
					} else
					if (obj->hasAnim) {
						DrawPartialDecal({(obj->x-PLAYER_COORDS[0])*32+WIDTH/2+(obj->anim->flipped?obj->anim->width:0),(obj->y-PLAYER_COORDS[1])*32+HEIGHT/2},obj->anim->spr,{(float)(obj->anim->getCurrentFrame().x),(float)(obj->anim->getCurrentFrame().y)},{(float)(obj->anim->width),(float)(obj->anim->height)},{(float)(obj->anim->flipped?-1:1),1},obj->col);
					} else 
					if (obj->hascut) {
						DrawPartialDecal({(obj->x-PLAYER_COORDS[0])*32+WIDTH/2+(obj->flipped?obj->size.x:0),(obj->y-PLAYER_COORDS[1])*32+HEIGHT/2},obj->spr,obj->spos,obj->size,{(float)(obj->flipped?-1:1),1},obj->col);
					} else {
						DrawDecal({(obj->x-PLAYER_COORDS[0])*32+WIDTH/2,(obj->y-PLAYER_COORDS[1])*32+HEIGHT/2},obj->spr,{1,1},obj->col);
					}
				}
			} else 
			if (obj->name.compare("EXIT")==0) {
				GradientFillRectDecal({(obj->x-PLAYER_COORDS[0])*32+WIDTH/2,(obj->y-PLAYER_COORDS[1])*32+HEIGHT/2},{32,32},Pixel(0,0,0,0),WHITE,WHITE,Pixel(0,0,0,0));
			}
		}
		if (IN_BATTLE_ENCOUNTER) {
			const vi2d CENTER = {(int)((float)((BATTLE_PLAYER_COORDS.x-PLAYER_COORDS[0])*32+WIDTH/2+(current_playerAnim->flipped?32:0))),(int)((float)((BATTLE_PLAYER_COORDS.y-PLAYER_COORDS[1])*32+HEIGHT/2))};
			for (int i=0;i<SEEDS.size();i++) {
				DrawDecal(SEEDS[i]->pos+CENTER,SEED_DECAL,{0.5,0.5});
			}
			for (int i=0;i<TREES.size();i++) {
				DrawDecal(TREES[i]->pos+CENTER,TREE_DECAL,{1,1});
			}
		}
		for (auto&enc:ENCOUNTERS) {
			if (enc.map==CURRENT_MAP) {
				int targetX=-1,targetY=-1;
				for (auto&ent:enc.entities) {
						if (BATTLE_STATE==battle::PLAYER_TARGET_SELECTION&&PLAYER_SELECTED_TARGET>=0&&CURRENT_ENCOUNTER.entities[PLAYER_SELECTED_TARGET]->hp>0&&CURRENT_ENCOUNTER.entities[PLAYER_SELECTED_TARGET]->x==ent->x&&CURRENT_ENCOUNTER.entities[PLAYER_SELECTED_TARGET]->y==ent->y) {
							DrawDecal({(enc.x+ent->x-PLAYER_COORDS[0])*32+WIDTH/2,(enc.y+ent->y-PLAYER_COORDS[1])*32+HEIGHT/2},TARGETING_CIRCLE,{(float)(ent->spr->sprite->width*ent->sprScale.x/32),(float)(ent->spr->sprite->height*ent->sprScale.y/32)},{255,210,0,255});
							DrawDecal({(enc.x+ent->x-PLAYER_COORDS[0])*32+WIDTH/2,(enc.y+ent->y-PLAYER_COORDS[1])*32+HEIGHT/2},ent->spr,ent->sprScale,{(uint8_t)((0.5*(float)sin(frameCount*4/60.0)+0.5)*80+175),(uint8_t)((0.5*(float)sin(frameCount*4/60.0)+0.5)*80+175),(uint8_t)((0.5*(float)sin(frameCount*4/60.0)+0.5)*80+175),255});
							targetX=ent->x;targetY=ent->y;
						} else {
							if (ent->hp>0) {//Outside battles.
								DrawDecal({(enc.x+ent->x-PLAYER_COORDS[0])*32+WIDTH/2,(enc.y+ent->y-PLAYER_COORDS[1])*32+HEIGHT/2},ent->spr,ent->sprScale);
							}
						}
						if (ent->hp>0) {
							DrawHealthbar({(enc.x+ent->x-PLAYER_COORDS[0])*32+WIDTH/2,(enc.y+ent->y-PLAYER_COORDS[1])*32+HEIGHT/2+ent->spr->sprite->height*ent->sprScale.y+2},ent->spr->sprite->width*ent->sprScale.x,(float)ent->hp/ent->maxhp,BLACK);
							DrawBuffs({(enc.x+ent->x-PLAYER_COORDS[0])*32+WIDTH/2,(enc.y+ent->y-PLAYER_COORDS[1])*32+HEIGHT/2+ent->spr->sprite->height*ent->sprScale.y+12},ent);
						}
				}
				if (targetX!=-1&&targetY!=-1) {
					DrawDecal({(enc.x+targetX-PLAYER_COORDS[0])*32+enc.entities[PLAYER_SELECTED_TARGET]->spr->sprite->width/2*enc.entities[PLAYER_SELECTED_TARGET]->sprScale.x+WIDTH/2-BATTLE_CARD_SELECTION->range,(enc.y+targetY-PLAYER_COORDS[1])*32+enc.entities[PLAYER_SELECTED_TARGET]->spr->sprite->height/2*enc.entities[PLAYER_SELECTED_TARGET]->sprScale.y+HEIGHT/2-BATTLE_CARD_SELECTION->range},TARGETING_RANGE_CIRCLE,{(float)(BATTLE_CARD_SELECTION->range*2/32.0),(float)(BATTLE_CARD_SELECTION->range*2/32.0)},{255,60,0,(uint8_t)((0.5*(float)sin(frameCount*4/60.0)+0.5)*100)});
				}
			}
		}
	}

	void DrawHealthbar(vf2d pos,float width,float pct,Pixel col) {
		GradientFillRectDecal(pos,{width*pct,8},RED,RED,Pixel((1/pct)*255,255*pct,0,255),Pixel((1/pct)*255,255*pct,0,255));
		DrawDecal(pos,HEALTHBAR_DECAL,{width/32,1},col);
	}

	void DrawTerminal() {
		const int TERMINAL_WIDTH=7;
		for (int i=0;i<26;i++) {
			std::string s(1,(char)('A'+i));
			DrawStringDecal({(float)((32*(i%TERMINAL_WIDTH))+16),(float)(16*(i/TERMINAL_WIDTH)+HEIGHT/2)},s);
		}
		std::string backspace(1,(char)('<'));
		DrawStringDecal({(float)((32*(26%TERMINAL_WIDTH))+16),(float)(16*(26/TERMINAL_WIDTH)+HEIGHT/2)},backspace);
		DrawStringDecal({(float)((32*(27%TERMINAL_WIDTH))+16),(float)(16*(27/TERMINAL_WIDTH)+HEIGHT/2)},"ENTER");
		DrawStringDecal({(float)((32*((TERMINAL_SELECTED_CHAR)%TERMINAL_WIDTH))+8),(float)(16*(TERMINAL_SELECTED_CHAR/TERMINAL_WIDTH)+HEIGHT/2)},">");
		std::string terminal_name=TERMINAL_INPUT;
		for (int i=0;i<MAX_TERMINAL_NAME_LENGTH-TERMINAL_INPUT.length();i++) {
			terminal_name+="*";
		}
		DrawStringDecal({(float)(WIDTH/2-GetTextSize(terminal_name).x*2/2),(float)(HEIGHT/3-GetTextSize(terminal_name).y*2/2)}, terminal_name,olc::WHITE,{2.0,2.0});

		GradientFillRectDecal({0,0},{WIDTH/2,HEIGHT/2},{20, 28, 22,ALPHA_SCREEN1},{20, 28, 22,ALPHA_SCREEN1},{20, 28, 22,ALPHA_SCREEN2},{20, 28, 22,ALPHA_SCREEN1});
		GradientFillRectDecal({WIDTH/2,0},{WIDTH/2,HEIGHT/2},{20, 28, 22,ALPHA_SCREEN1},{20, 28, 22,ALPHA_SCREEN2},{20, 28, 22,ALPHA_SCREEN1},{20, 28, 22,ALPHA_SCREEN1});
		GradientFillRectDecal({0,HEIGHT/2},{WIDTH/2,HEIGHT/2},{20, 28, 22,ALPHA_SCREEN1},{20, 28, 22,ALPHA_SCREEN1},{20, 28, 22,ALPHA_SCREEN1},{20, 28, 22,ALPHA_SCREEN2});
		GradientFillRectDecal({WIDTH/2,HEIGHT/2},{WIDTH/2,HEIGHT/2},{20, 28, 22,ALPHA_SCREEN2},{20, 28, 22,ALPHA_SCREEN1},{20, 28, 22,ALPHA_SCREEN1},{20, 28, 22,ALPHA_SCREEN1});
	}

	int getPlantId(int x, int y) {
		return ((int)x-8)%4+((int)y-2)*4;
	}

	int getPlantStatusWithAbsoluteCoords(int x,int y) {
		return plantState>>getPlantId(x+8,y+2)*2&0b11;
	}

	//This uses MAP COORDS for determining bit location. The plants are offset by (X+8,Y+2)!! Use getPlantStatusWithAbsoluteCoords() to translate to map coords.
	int getPlantStatus(int x,int y) {
		return plantState>>getPlantId(x,y)*2&0b11;
	}

	void setPlantStatusWithAbsoluteCoords(int x,int y,char state) {
		setPlantStatus(x+8,y+2,state);
	}

	//This uses MAP COORDS for determining bit location. The plants are offset by (X+8,Y+2)!! Use setPlantStatusWithAbsoluteCoords() to translate to map coords.
	void setPlantStatus(int x,int y,char state) {
		int mask=0b11111111111111111111111111111111;
		mask-=3<<getPlantId(x,y)*2;
		plantState&=mask;
		plantState|=state<<getPlantId(x,y)*2;
	}

	void DrawDialogBox(const vi2d &pos, const vi2d &size, Pixel p = WHITE, Pixel p2 = DARK_GREY, Pixel p3 = VERY_DARK_GREY) {
		FillRectDecal({(float)pos.x,(float)pos.y},size,p2);
		FillRectDecal({(float)pos.x+1,(float)pos.y+1},{(float)size.x-2,(float)size.y-2},p);
		FillRectDecal({(float)pos.x+2,(float)pos.y+2},{(float)size.x-4,(float)size.y-4},p3);
		FillRectDecal({(float)pos.x+3,(float)pos.y+3},{(float)size.x-5,(float)size.y-5},p);
		Draw({pos.x,pos.y},BLACK);
		Draw({pos.x+size.x,pos.y+size.y},BLACK);
		Draw({pos.x+size.x,pos.y},BLACK);
		Draw({pos.x,pos.y+size.y},BLACK);
	}

	void fadeOut() {
		fade=true;
	}
	void fadeIn() {
		fade=false;
	}

	void audioFadeOut() {
		audioFade=true;
	}
	void audioFadeIn() {
		audioFade=false;
	}
	sound::PlayingWave playMusic(sound::Wave*sound,bool loop=true,float spd=1) {
		if (!queueBGMPlayback) {
			audioFadeIn();
		}
		if (SOUND_IS_PLAYING) {
			stopMusic();
		}
		SOUND_IS_PLAYING=true;
		CURRENT_BGM=engine.PlayWaveform(sound,loop,spd);
		return CURRENT_BGM;
	} 
	void PlaySound(sound::Wave* sound,bool loop=false,float spd=1) {
		if (!queueBGMPlayback) {
			audioFadeIn();
		}
		if (LAST_FRAME_SOUND_PLAYED!=frameCount) {
			engine.PlayWaveform(sound,loop,spd);
			LAST_FRAME_SOUND_PLAYED=frameCount;
		}
	} 

	void stopMusic() {
		engine.StopWaveform(CURRENT_BGM);
		SOUND_IS_PLAYING=false;
	}

	void StopCutscene() {
		EndCutscene();
	}

	void EndCutscene() {
		CURRENT_CUTSCENE=cutscene::NONE;
		ClearAllTemporaryObjects();
	}

	void DisplayMessageBox(int dialogNumber) {
		performCropUpdate(48);
		messageBoxVisible=true;
		messageBoxCursor=0;
		std::string split1=STORY_DIALOG[dialogNumber].substr(0,STORY_DIALOG[dialogNumber].find('\n')); //Unused for now.
		std::string split2=STORY_DIALOG[dialogNumber].substr(STORY_DIALOG[dialogNumber].find('\n')+1,STORY_DIALOG[dialogNumber].find('\n',STORY_DIALOG[dialogNumber].find('\n')+1)-(STORY_DIALOG[dialogNumber].find('\n')+1));
		std::string split3=STORY_DIALOG[dialogNumber].substr(STORY_DIALOG[dialogNumber].find('\n',STORY_DIALOG[dialogNumber].find('\n')+1)+1,STORY_DIALOG[dialogNumber].length()-(STORY_DIALOG[dialogNumber].find('\n',STORY_DIALOG[dialogNumber].find('\n')+1)+1));
		messageBoxSpeaker=(split2.compare("$PLAYER")==0)?split2.replace(0,split2.length(),PLAYER_NAME):split2;
		messageBoxText="";
		messageBoxRefText=split3;
	}

	void LoadMap(Map*map) {
		std::ifstream f("assets/maps/"+map->filename);
		std::string data;
		CURRENT_MAP=map;
		MAP_WIDTH=MAP_HEIGHT=-1;
		if (MAP!=NULL) {
			for (int y=0;y<MAP_HEIGHT;y++) {
				free(MAP[y]);
			}
			free(MAP);
			MAP=NULL;
		}
		for (int i=0;i<OBJECTS.size();i++) {
			delete OBJECTS[i];
		}
		OBJECTS.clear();
		for (int i=0;i<8;i++) {
			CUTSCENE_OBJS[i]=nullptr;
		}

		int y=0;
		if (f.is_open()) {
			while (f.good()) {
				f>>data;
				if (MAP_WIDTH==-1) {
					std::stringstream stream(data);
					stream>>MAP_WIDTH;
				} else
				if (MAP_HEIGHT==-1) {
					std::stringstream stream(data);
					stream>>MAP_HEIGHT;
				} else 
				if (y<MAP_HEIGHT) {
					if (MAP==NULL) {
						MAP=(int**)malloc(sizeof(int**)*MAP_HEIGHT);
					}
					MAP[y]=(int*)malloc(sizeof(int*)*MAP_WIDTH);
					for (int i=0;i<data.length();i++) {
						MAP[y][i]=data[i]-'0';
					}
					y++;
				} else {
					std::stringstream split1(data.substr(0,data.find(';')));
					std::stringstream split2(data.substr(data.find(';')+1,data.find(';',data.find(";")+1)-(data.find(';')+1)));
					float x,y;
					split1>>x;
					split2>>y;
					bool alreadyCollected=false;
					bool alreadyRepaired=false;
					for (int i=0;i<COLLECTED_ITEMS.size();i++) {
						if (x==COLLECTED_ITEMS[i].x&&y==COLLECTED_ITEMS[i].y) {
							alreadyCollected=true;
							break;
						}
					}
					for (int i=0;i<REPAIRED_ROVERS.size();i++) {
						if (x==REPAIRED_ROVERS[i].x&&y==REPAIRED_ROVERS[i].y) {
							alreadyRepaired=true;
							break;
						}
					}
					if (!alreadyCollected) {
						Object*obj = new Object();
						obj->x=x;
						obj->y=y;
						std::string split3 = data.substr(data.find(';',data.find(";")+1)+1,data.length()-(data.find(';',data.find(";")+1)+1));
						if (split3.compare("NULL")!=0) {
							if (alreadyRepaired) {
								obj->spr=ROVER_DECAL;
							} else {
								obj->spr=BASE_OBJECTS[split3]->spr;
							}
							obj->col=BASE_OBJECTS[split3]->col;
							if (BASE_OBJECTS[split3]->hasanim) {
								obj->hasAnim=true;
								obj->anim=BASE_OBJECTS[split3]->anim;
							} else 
							if (BASE_OBJECTS[split3]->hascut){
								obj->hascut=true;
								obj->spos=BASE_OBJECTS[split3]->spos;
								obj->size=BASE_OBJECTS[split3]->size;
							}
						} else {
							obj->spr=NULL;
						}
						obj->name=split3;
						printf("Loaded object %s: (%f,%f)\n",split3.c_str(),obj->x,obj->y);
						OBJECTS.push_back(obj);
					}
				}
			}
		}
		printf("Loaded %dx%d map:\n",MAP_WIDTH,MAP_HEIGHT);
		for (int y=0;y<MAP_HEIGHT;y++) {
			for (int x=0;x<MAP_WIDTH;x++) {
				printf("%d",MAP[y][x]);
			}
			printf("\n");
		}
		f.close();
	}

	void changeAnimation(Animation*anim) {
		current_playerAnim=anim;
		current_playerAnim->frame=0;
	}

	bool collidesWithPlayer(Object*obj) {
		if (!IN_BATTLE_ENCOUNTER) {
			if (obj->hasAnim) {
				return PLAYER_COORDS[0]>=obj->x&&PLAYER_COORDS[0]<=obj->x+obj->anim->width/32.0&&
				PLAYER_COORDS[1]>=obj->y&&PLAYER_COORDS[1]<=obj->y+obj->anim->height/32.0;
			} else {
				if (obj->spr==NULL) {
					return PLAYER_COORDS[0]>=obj->x&&PLAYER_COORDS[0]<=obj->x+1&&
					PLAYER_COORDS[1]>=obj->y&&PLAYER_COORDS[1]<=obj->y+1;
				} else {
					return PLAYER_COORDS[0]>=obj->x&&PLAYER_COORDS[0]<=obj->x+obj->spr->sprite->width/32.0&&
					PLAYER_COORDS[1]>=obj->y&&PLAYER_COORDS[1]<=obj->y+obj->spr->sprite->height/32.0;
				}
			}
		}
		return false;
	}

	bool collidesWithPlayer(Encounter obj) {
		return PLAYER_COORDS[0]>=obj.x&&PLAYER_COORDS[0]<=obj.x+WIDTH/32&&
		PLAYER_COORDS[1]>=obj.y&&PLAYER_COORDS[1]<=obj.y+HEIGHT/32;
	}

	void DrawWrappedText(vf2d pos,std::string text,int targetWidth,Pixel col,vf2d scale) {
		std::string wrappedText;
		int marker=0;
		while (marker<text.length()) {
			wrappedText+=text[marker];
			if (GetTextSize(wrappedText).x*scale.x>targetWidth) {
				int tempMarker=marker;
				while (wrappedText[tempMarker]!=' ') {
					wrappedText.erase(tempMarker--);
				}
				wrappedText.erase(tempMarker++);
				wrappedText+='\n';
				while (tempMarker<marker+1) {
					wrappedText+=text[tempMarker++];
				}
			}
			marker++;
		}
		DrawStringDecal(pos,wrappedText,col,scale);
	}

	void DrawWrappedPropText(vf2d pos,std::string text,int targetWidth,Pixel col,vf2d scale) {
		std::string wrappedText;
		int marker=0;
		while (marker<text.length()) {
			wrappedText+=text[marker];
			if (GetTextSizeProp(wrappedText).x*scale.x>targetWidth) {
				int tempMarker=marker;
				while (wrappedText[tempMarker]!=' ') {
					wrappedText.erase(tempMarker--);
				}
				wrappedText.erase(tempMarker++);
				wrappedText+='\n';
				while (tempMarker<marker+1) {
					wrappedText+=text[tempMarker++];
				}
			}
			marker++;
		}
		DrawStringPropDecal(pos,wrappedText,col,scale);
	}

	bool playerCanMove() {
		return PLAYER_HP>0&&(GAME_STATE==GAMEWORLD||GAME_STATE==IN_SPACE)&&CURRENT_CUTSCENE==cutscene::NONE&&!messageBoxVisible&&!IN_BATTLE_ENCOUNTER;
	}

	void DrawCard(WEATHER_POWER*data,vf2d offset={0,0},vf2d scale={1,1},float darknessFactor=1.0) {
		float dimColor=data->playerOwnCount==0?0.4:1;
		
		GradientFillRectDecal({(WIDTH/6)*scale.x+offset.x,(HEIGHT/6)*scale.y+offset.y},{WIDTH/3*scale.x,HEIGHT/6*2*scale.y},data->bgcol*darknessFactor*dimColor,data->bgcol*darknessFactor*dimColor,Pixel(72, 160, 212,0),data->bgcol*darknessFactor*dimColor);
		GradientFillRectDecal({(WIDTH/6*3+1)*scale.x+offset.x,(HEIGHT/6)*scale.y+offset.y},{WIDTH/3*scale.x,HEIGHT/6*2*scale.y},data->bgcol*darknessFactor*dimColor,Pixel(72, 160, 212,0),data->bgcol*darknessFactor*dimColor,data->bgcol*darknessFactor*dimColor);
		GradientFillRectDecal({(WIDTH/6)*scale.x+offset.x,(HEIGHT/6*3)*scale.y+offset.y},{WIDTH/3*scale.x,HEIGHT/6*2*scale.y},data->bgcol*darknessFactor*dimColor,data->bgcol*darknessFactor*dimColor,data->bgcol*darknessFactor*dimColor,Pixel(72, 160, 212,0));
		GradientFillRectDecal({(WIDTH/6*3+1)*scale.x+offset.x,(HEIGHT/6*3)*scale.y+offset.y},{WIDTH/3*scale.x,HEIGHT/6*2*scale.y},Pixel(72, 160, 212,0),data->bgcol*darknessFactor*dimColor,data->bgcol*darknessFactor*dimColor,data->bgcol*darknessFactor*dimColor);
		
		DrawPartialDecal({(WIDTH/2-data->anim->width/2*3)*scale.x+offset.x,(HEIGHT/6+16-data->anim->height/2)*scale.y+offset.y},data->anim->spr,data->anim->getCurrentFrame(),{(float)(data->anim->width),(float)(data->anim->height)},{3*scale.x,3*scale.y},WHITE*darknessFactor*dimColor);
		for (int x=-1;x<=1;x++) {
			for (int y=-1;y<=1;y++) {
				if (x!=0&&y!=0) {
					DrawStringPropDecal({(WIDTH/6+4+x)*scale.x+offset.x,(HEIGHT/2+8+y)*scale.y+offset.y},data->name,BLACK,{2*scale.x,2*scale.y});
				}
			}
		}
		DrawStringPropDecal({(WIDTH/6+4)*scale.x+offset.x,(HEIGHT/2+8)*scale.y+offset.y},data->name,data->textcol*darknessFactor*dimColor,{2*scale.x,2*scale.y});
		DrawWrappedPropText({(WIDTH/6+4+1)*scale.x+offset.x,(HEIGHT/2+24+1)*scale.y+offset.y},data->description,(WIDTH/3*2-8)*scale.x,BLACK,{scale.x,scale.y});
		DrawWrappedPropText({(WIDTH/6+4)*scale.x+offset.x,(HEIGHT/2+24)*scale.y+offset.y},data->description,(WIDTH/3*2-8)*scale.x,Pixel(227, 245, 255,255)*darknessFactor*dimColor,{scale.x,scale.y});
		for (int x=-1;x<=1;x++) {
			for (int y=-1;y<=1;y++) {
				if (x!=0&&y!=0) {
					DrawStringPropDecal({(float)((WIDTH/6+4+WIDTH/3*2-8-GetTextSizeProp(std::to_string(data->playerOwnCount)+" uses remaining.").x*1.5+x)*scale.x+offset.x),(float)((HEIGHT/6*3+HEIGHT/6*2-GetTextSizeProp(std::to_string(data->playerOwnCount)+" uses remaining.").y*1.5-4+y))*scale.y+offset.y},std::to_string(data->playerOwnCount)+" uses remaining.",olc::BLACK,{(float)(1.5*scale.x),(float)(1.5*scale.y)});
				}
			}
		} 
		DrawStringPropDecal({(float)((WIDTH/6+4+WIDTH/3*2-8-GetTextSizeProp(std::to_string(data->playerOwnCount)+" uses remaining.").x*1.5)*scale.x+offset.x),(float)((HEIGHT/6*3+HEIGHT/6*2-GetTextSizeProp(std::to_string(data->playerOwnCount)+" uses remaining.").y*1.5-4)*scale.y+offset.y)},std::to_string(data->playerOwnCount)+" uses remaining.",(data->playerOwnCount==0)?olc::RED:olc::WHITE*darknessFactor,{(float)(1.5*scale.x),(float)(1.5*scale.y)});
		
	}

	void effectRadius(vi2d coords,WEATHER_POWER*power,bool playerForce) {
		int finalDamage=power->damage+rand()%power->damageRoll*sign(power->damage);
		effectRadius(coords,power,finalDamage,playerForce);
	}

	void effectRadius(vi2d coords,WEATHER_POWER*power,int finalDamage,bool playerForce) {
		std::cout<<"Emitting effect radius for "<<power->name<<" for PlayerForce:"<<playerForce<<"\n";
		if (finalDamage<0) {
			//This is a healing effect.
			if (playerForce) {
				if (power->damage==-1002) {
					//Instead heal for their max health.
					PLAYER_MAXHP+=5;
					finalDamage=-PLAYER_MAXHP;
					foodCount--;
				} else
				if (power->damage==-1001) {
					FOOD_REGEN_TURNS=4;
					finalDamage=(int)(-PLAYER_MAXHP*0.33*(BATTLE_DROUGHT_ACTIVE?0.5:1));
					foodCount--;
				}
				PLAYER_HP=std::clamp(PLAYER_HP-finalDamage,0,PLAYER_MAXHP);
				CreateDisplayNumber(finalDamage,CURRENT_ENCOUNTER.x+CURRENT_ENCOUNTER.playerX,CURRENT_ENCOUNTER.y+CURRENT_ENCOUNTER.playerY,frameCount);
			} else {
				for (int i=0;i<CURRENT_ENCOUNTER.entities.size();i++) {
					finalDamage=power->damage+rand()%power->damageRoll*sign(power->damage);
					Entity*ent=CURRENT_ENCOUNTER.entities[i];
					std::cout<<"Distance is "<<distancetoCoords({(float)((ent->x+CURRENT_ENCOUNTER.x)*32)+ent->spr->sprite->width*ent->sprScale.x/2,(float)((ent->y+CURRENT_ENCOUNTER.y)*32)+ent->spr->sprite->height*ent->sprScale.y/2},coords*32)<<"m.\n";
					if (distancetoCoords({(float)((ent->x+CURRENT_ENCOUNTER.x)*32)+ent->spr->sprite->width*ent->sprScale.x/2,(float)((ent->y+CURRENT_ENCOUNTER.y)*32)+ent->spr->sprite->height*ent->sprScale.y/2},coords*32)<=power->range&&!ent->hidden) {
						ent->hp=std::clamp(ent->hp-finalDamage,0,ent->maxhp);
						CreateDisplayNumber(finalDamage,ent->x+CURRENT_ENCOUNTER.x+ent->spr->sprite->width*ent->sprScale.x/2/32,ent->y+CURRENT_ENCOUNTER.y+ent->spr->sprite->height*ent->sprScale.y/2/32,frameCount);
						ent->damageFrame=frameCount;
					}
				}
			}
		} else {
			//Damaging effect.
			if (playerForce) {
				for (int i=0;i<CURRENT_ENCOUNTER.entities.size();i++) {
					finalDamage=power->damage+rand()%power->damageRoll*sign(power->damage);
					Entity*ent=CURRENT_ENCOUNTER.entities[i];
					//std::cout<<"Distance was "<<distancetoCoords({(float)((ent->x+CURRENT_ENCOUNTER.x)*32),(float)((ent->y+CURRENT_ENCOUNTER.y)*32)},coords*32)<<"\n";
					if (ent->hp>0&&distancetoCoords({(float)((ent->x+CURRENT_ENCOUNTER.x)*32)+ent->spr->sprite->width*ent->sprScale.x/2,(float)((ent->y+CURRENT_ENCOUNTER.y)*32)+ent->spr->sprite->height*ent->sprScale.y/2},coords*32)<=power->range&&!ent->hidden) {
						ent->hp=std::clamp(ent->hp-finalDamage,0,ent->maxhp);
						CreateDisplayNumber(finalDamage,ent->x+CURRENT_ENCOUNTER.x+ent->spr->sprite->width*ent->sprScale.x/2/32,ent->y+CURRENT_ENCOUNTER.y+ent->spr->sprite->height*ent->sprScale.y/2/32,frameCount);
						ent->damageFrame=frameCount;
					}
				}
			} else {
				if (power->dealsPctDmg) {
					finalDamage=PLAYER_HP*power->pctDmg;
					PLAYER_HP=std::clamp(PLAYER_HP-finalDamage,0,PLAYER_MAXHP);
				} else {
					PLAYER_HP=std::clamp(PLAYER_HP-finalDamage,0,PLAYER_MAXHP);
				}
				CreateDisplayNumber(finalDamage,CURRENT_ENCOUNTER.x+CURRENT_ENCOUNTER.playerX,CURRENT_ENCOUNTER.y+CURRENT_ENCOUNTER.playerY,frameCount);
			}
		}
	}

	double distancetoCoords(vf2d c1,vf2d c2) {
		return sqrt(pow(c1.x-c2.x,2)+pow(c2.y-c1.y,2));
	}

	int sign(int x) {
		return (x > 0) - (x < 0);
	}

	int rand() {
		srand(time(NULL)+RAND_CALLS);
		int randNumb = std::rand();
		RAND_CALLS=randNumb;
		return randNumb;
	}

	void performCropUpdate(int chanceToRegrow) {
		for (int x=0;x<4;x++) {
			for (int y=0;y<4;y++) {
				if (rand()%chanceToRegrow==0) {
					setPlantStatusWithAbsoluteCoords(x,y,std::clamp(getPlantStatusWithAbsoluteCoords(x,y)+1,0,2));
				}
			}
		}
	}

	void setupBattleTurns() {
		//Remove the card from inventory.
		BATTLE_CARD_SELECTION->playerOwnCount--;
		BATTLE_STATE=battle::ENEMY_SELECTION;
		for (int i=0;i<CURRENT_ENCOUNTER.entities.size();i++) {
			if (CURRENT_ENCOUNTER.entities[i]->hp>0) {
				if (CURRENT_ENCOUNTER.entities[i]->fixedTurnOrder) {
					CURRENT_ENCOUNTER.entities[i]->selectedMove=CURRENT_ENCOUNTER.entities[i]->moveSet[CURRENT_ENCOUNTER.entities[i]->fixedTurnOrderInd++];
					if (CURRENT_ENCOUNTER.entities[i]->fixedTurnOrderInd>=CURRENT_ENCOUNTER.entities[i]->moveSet.size()) {
						CURRENT_ENCOUNTER.entities[i]->fixedTurnOrderInd=0;
					}
				} else {
					CURRENT_ENCOUNTER.entities[i]->selectedMove=CURRENT_ENCOUNTER.entities[i]->moveSet[rand()%CURRENT_ENCOUNTER.entities[i]->moveSet.size()];
				}
				CURRENT_ENCOUNTER.entities[i]->turnComplete=false;
			}
		}
		PLAYER_TURN_COMPLETE=false;
		BATTLE_STATE=battle::MOVE_RESOLUTION;
		//Seed Storm has prio.
		if (!PLAYER_TURN_COMPLETE&&isPriorityMove(BATTLE_CARD_SELECTION)&&PETRIFY_TURNS==0) {
			turnOrder.push(-1);
			PLAYER_TURN_COMPLETE=true;
		}
		for (int i=0;i<CURRENT_ENCOUNTER.entities.size();i++) {
			if (CURRENT_ENCOUNTER.entities[i]->hp>0&&!CURRENT_ENCOUNTER.entities[i]->turnComplete&&
				((isPriorityMove(CURRENT_ENCOUNTER.entities[i]->selectedMove)&&CURRENT_ENCOUNTER.entities[i]->speed>=0)||CURRENT_ENCOUNTER.entities[i]->speed==1)) {
				turnOrder.push(i);
				CURRENT_ENCOUNTER.entities[i]->turnComplete=true;
			}
		}
		//Healing has half prio for the player.
		if (rand()%2==0&&!PLAYER_TURN_COMPLETE&&isFoodMove(BATTLE_CARD_SELECTION)&&PETRIFY_TURNS==0) {
			turnOrder.push(-1);
			PLAYER_TURN_COMPLETE=true;
		}
		//Otherwise, every enemy has a chance to go before the player unless they are slowed, then they always go after.
		for (int i=0;i<CURRENT_ENCOUNTER.entities.size();i++) {
			Entity*ent = CURRENT_ENCOUNTER.entities[i];
			if (rand()%2==0&&!PLAYER_TURN_COMPLETE&&isFoodMove(BATTLE_CARD_SELECTION)&&PETRIFY_TURNS==0) {
				turnOrder.push(-1);
				PLAYER_TURN_COMPLETE=true;
			}
			if (ent->hp>0&&!ent->turnComplete&&
				ent->speed>=0&&rand()%2==0&&!ent->selectedMove->lowPriority) {
				turnOrder.push(i);
				ent->turnComplete=true;
			}
		}
		if (!PLAYER_TURN_COMPLETE&&PETRIFY_TURNS==0) {
			turnOrder.push(-1);
			PLAYER_TURN_COMPLETE=true;
		}
		//Finally, any enemies that haven't gone will now go.
		for (int i=0;i<CURRENT_ENCOUNTER.entities.size();i++) {
			Entity*ent = CURRENT_ENCOUNTER.entities[i];
			if (ent->hp>0&&!ent->turnComplete) {
				turnOrder.push(i);
				ent->turnComplete=true;
			}
		}
		BATTLE_STATE=battle::PERFORM_TURN;
		BATTLE_CURRENT_TURN_ENTITY=turnOrder.front();
		turnOrder.pop();
		BATTLE_STATE=battle::WAIT_TURN_ANIMATION;
		EFFECT_TIMER=0;
	}

	void displayPowerInfo(WEATHER_POWER*POWER){
		PlayCutscene(cutscene::NODE_COLLECT_CUTSCENE);
		CUTSCENE_DISPLAYED_CARD=POWER;
		PlaySound(&SOUND_SELECT,false,0.9);
	}

	void applyPixelEffect(ParticleEffect*effect,vf2d pos,float startingTransparency) {
		applyPixelEffect(
			{(pos.x-PLAYER_COORDS[0])*32+WIDTH/2-effect->effectSize.x/2,(pos.y-PLAYER_COORDS[1])*32+HEIGHT/2-effect->effectSize.y/2},
			effect->effectSize,
			effect->pos_low,
			effect->pos_high,
			effect->size_low,
			effect->size_high,
			effect->spd_low,
			effect->spd_high,
			effect->col_low,
			effect->col_high,
			effect->amt,
			effect->foregroundColor,startingTransparency
		);
	}

	void applyPixelEffect(WEATHER_POWER*power,vf2d pos,float startingTransparency) {
		applyPixelEffect(
			{(pos.x-PLAYER_COORDS[0])*32+WIDTH/2-power->effect->effectSize.x/2,(pos.y-PLAYER_COORDS[1])*32+HEIGHT/2-power->effect->effectSize.y/2},
			power->effect->effectSize,
			power->effect->pos_low,
			power->effect->pos_high,
			power->effect->size_low,
			power->effect->size_high,
			power->effect->spd_low,
			power->effect->spd_high,
			power->effect->col_low,
			power->effect->col_high,
			power->effect->amt,
			power->effect->foregroundColor,startingTransparency
		);
	}

	void applyPixelEffect(vf2d effectPos,vf2d effectSize,vf2d pos_low,vf2d pos_high,vf2d size_low,vf2d size_high,vf2d spd_low,vf2d spd_high,olc::Pixel col_low,olc::Pixel col_high,int amt,olc::Pixel foregroundColor,float startingTransparency) {
		for (int i=0;i<amt;i++) {
			pixels[i]->pos={range(pos_low.x,pos_high.x),range(pos_low.y,pos_high.y)};
			pixels[i]->size={range(size_low.x,size_high.x),range(size_low.y,size_high.y)};
			//std::cout<<std::to_string(pixels[i]->size.x)<<","<<std::to_string(pixels[i]->size.y)<<"\n";
			pixels[i]->spd={range(spd_low.x,spd_high.x),range(spd_low.y,spd_high.y)};
			pixels[i]->r=range(col_low.r,col_high.r);
			pixels[i]->g=range(col_low.g,col_high.g);
			pixels[i]->b=range(col_low.b,col_high.b);
			pixels[i]->a=range(col_low.a,col_high.a);
			pixels[i]->o_a=range(col_low.a,col_high.a);
		}
		ORIGINAL_FOREGROUND_EFFECT_COLOR=FOREGROUND_EFFECT_COLOR=foregroundColor;
		PIXEL_POS=effectPos;
		PIXEL_SIZE=effectSize;
		PIXEL_LIMIT=amt;
		PIXEL_EFFECT_TRANSPARENCY=startingTransparency;
	}

	void clearPixelEffect() {
		PIXEL_LIMIT=0;
		PIXEL_EFFECT_TRANSPARENCY=0;
	}

	//Returns a number between 0.0 and X.
	float random(float X) {
		return (float)rand()/RAND_MAX * X;
	}

	float range(float val1,float val2) {
		return random(val2-val1)+val1;
	}

	vf2d GetMapFileCoords(int ln,int col) {
		return {(float)col-1,(float)ln-3};
	}

	//Basically, click on a spot in the text editor and this function translates it into the proper teleport coords so you don't have to.
	void TeleportToMapFileCoords(int ln,int col) {
		vi2d teleLoc = GetMapFileCoords(ln,col);
		PLAYER_COORDS[0]=teleLoc.x+0.5;
		PLAYER_COORDS[1]=teleLoc.y+0.5;
	}

	Object*CreateObject(vf2d pos,Decal*spr,bool temporary=false) {
		Object*newobj;
		OBJECTS.push_back(newobj=new Object(spr));
		newobj->x=pos.x;
		newobj->y=pos.y;
		newobj->tempObj=temporary;
		return newobj;
	}
	Object*CreateObject(vf2d pos,Decal*spr,Animation*anim,bool temporary=false) {
		Object*newobj;
		OBJECTS.push_back(newobj=new Object(spr,anim));
		newobj->x=pos.x;
		newobj->y=pos.y;
		newobj->tempObj=temporary;
		return newobj;
	}
	Object*CreateObject(vf2d pos,Decal*spr,vi2d spos,vi2d size,bool temporary=false) {
		Object*newobj;
		OBJECTS.push_back(newobj=new Object(spr,spos,size));
		newobj->x=pos.x;
		newobj->y=pos.y;
		newobj->tempObj=temporary;
		return newobj;
	}
	void ClearAllTemporaryObjects() {
		for (int i=0;i<8;i++) {
			for (int j=0;j<OBJECTS.size();j++) {
				if (OBJECTS[j]==CUTSCENE_OBJS[i]) {
					if (CUTSCENE_OBJS[i]!=nullptr&&CUTSCENE_OBJS[i]->tempObj) {
						OBJECTS.erase(OBJECTS.begin()+j--);
						delete CUTSCENE_OBJS[i];
						std::cout<<"Erased at position "<<j<<".\n";
						break;
					}
				}
			}
		}
	}
	//Returns true if the camera reached the target position.
	bool MoveCameraTowardsPoint(vf2d pos,float spd=BATTLE_CAMERA_SCROLL_SPD) {
		bool reachedPosition=true;
		if (PLAYER_COORDS[0]!=pos.x) {
			if (PLAYER_COORDS[0]<pos.x) {
				PLAYER_COORDS[0]+=spd;
				if (PLAYER_COORDS[0]>pos.x) {
					PLAYER_COORDS[0]=pos.x;
				}
			} else {
				PLAYER_COORDS[0]-=spd;
				if (PLAYER_COORDS[0]<pos.x) {
					PLAYER_COORDS[0]=pos.x;
				}
			}
			reachedPosition=false;
		}
		if (PLAYER_COORDS[1]!=pos.y) {
			if (PLAYER_COORDS[1]<pos.y) {
				PLAYER_COORDS[1]+=spd;
				if (PLAYER_COORDS[1]>pos.y) {
					PLAYER_COORDS[1]=pos.y;
				}
			} else {
				PLAYER_COORDS[1]-=spd;
				if (PLAYER_COORDS[1]<pos.y) {
					PLAYER_COORDS[1]=pos.y;
				}
			}
			reachedPosition=false;
		}
		return reachedPosition;
	}
	bool MoveObjectTowardsPoint(vf2d pos,Object*obj,MOVEMENT_PRIORITY movementStyle,float spd=BATTLE_CAMERA_SCROLL_SPD,bool secondRun=false) {
		bool reachedPosition=true;
		if (movementStyle==HORZ_FIRST||movementStyle==BOTH) {
			if (obj->x!=pos.x) {
				if (obj->x<pos.x) {
					obj->x+=spd;
					if (obj->x>pos.x) {
						obj->x=pos.x;
					}
				} else {
					obj->x-=spd;
					if (obj->x<pos.x) {
						obj->x=pos.x;
					}
				}
				reachedPosition=false;
			} else 
			if (!secondRun&&movementStyle!=BOTH) {
				MoveObjectTowardsPoint(pos,obj,VERT_FIRST,spd,true);
			}
		}
		if (movementStyle==VERT_FIRST||movementStyle==BOTH) {
			if (obj->y!=pos.y) {
				if (obj->y<pos.y) {
					obj->y+=spd;
					if (obj->y>pos.y) {
						obj->y=pos.y;
					}
				} else {
					obj->y-=spd;
					if (obj->y<pos.y) {
						obj->y=pos.y;
					}
				}
				reachedPosition=false;
			} else 
			if (!secondRun&&movementStyle!=BOTH) {
				MoveObjectTowardsPoint(pos,obj,HORZ_FIRST,spd,true);
			}
		}
		return reachedPosition;
	}

	void ResetTerminal(std::string text){
		CUTSCENE_CONSOLE_TEXT.clear();
		textInd=0;
		CONSOLE_REF_TEXT=text;
	}

	bool isPriorityMove(WEATHER_POWER*power) {
		return 
			power->name.compare("Seed Storm")==0||
			power->name.compare("Seed of Life")==0||
			power->name.compare("Petal Storm")==0;
	}

	bool isBuffMove(WEATHER_POWER*power) { //ONLY Buff moves with no damaging/healing effects go in here.
		return 
			power->name.compare("Hide")==0||
			power->name.compare("Hyper Zap")==0||
			power->name.compare("Glare")==0||
			power->name.compare("Petrify")==0||
			power->name.compare("Radioactive Transmission")==0;
	}

	bool isFoodMove(WEATHER_POWER*power) {
		return 
			power->name.compare("Meal")==0||
			power->name.compare("Snack")==0;
	}

	void addSeeds(int seedCount) {
		for (int i=0;i<seedCount;i++) {
			SEEDS.push_back(new Seed({range(-32,32),range(-32,32)}));
		}
	}

	void clearSeeds() {
		for (int i=0;i<SEEDS.size();i++) {
			delete SEEDS[i];
		}
		SEEDS.clear();
	}

	void addTrees(int treeCount) {
		for (int i=0;i<treeCount;i++) {
			TREES.push_back(new Seed({range(-64,64),range(-64,64)}));
		}
	}

	void clearTrees() {
		for (int i=0;i<TREES.size();i++) {
			delete TREES[i];
		}
		TREES.clear();
	}

	DisplayNumber*CreateDisplayNumber(int numb,float x,float y,int frameCount) {
		DisplayNumber*number=new DisplayNumber(numb,x+range(-0.3,0.3),y+range(-0.1,0),frameCount);
		BATTLE_DISPLAY_NUMBERS.push_back(number);
		return number;
	}

	void updatePlayerState() {
		PREV_PLAYERSTATE->x=PLAYER_COORDS[0];
		PREV_PLAYERSTATE->y=PLAYER_COORDS[1];
		PREV_PLAYERSTATE->foodCount=foodCount;
		PREV_PLAYERSTATE->playerHP=PLAYER_HP;
		PREV_PLAYERSTATE->playerMaxHP=PLAYER_MAXHP;
		for (int i=0;i<WEATHER_POWER_COUNT;i++) {
			PREV_PLAYERSTATE->weatherPowerCounts[i]=WEATHER_POWERS[i]->playerOwnCount;
		}
		for (int i=0;i<128;i++) {
			PREV_PLAYERSTATE->gameFlags[i]=GAME_FLAGS[i];
		}
	}

	void LoadPreviousPlayerState() {
		PLAYER_COORDS[0]=PREV_PLAYERSTATE->x;
		PLAYER_COORDS[1]=PREV_PLAYERSTATE->y;
		foodCount=PREV_PLAYERSTATE->foodCount;
		PLAYER_HP=PREV_PLAYERSTATE->playerHP;
		PLAYER_MAXHP=PREV_PLAYERSTATE->playerMaxHP;
		for (int i=0;i<WEATHER_POWER_COUNT;i++) {
			WEATHER_POWERS[i]->playerOwnCount=PREV_PLAYERSTATE->weatherPowerCounts[i];
		}
		for (int i=0;i<128;i++) {
			GAME_FLAGS[i]=PREV_PLAYERSTATE->gameFlags[i];
		}
 	}

	void resetBattleState(){
		IN_BATTLE_ENCOUNTER=false;
		BATTLE_CARD_SELECTION_IND=0;
		PLAYER_SELECTED_TARGET=-1;
		BATTLE_STATE=battle::NONE;
		std::cout<<"Battle State set to "<<BATTLE_STATE<<"\n";
		BATTLE_DROUGHT_ACTIVE=false;
		FOOD_REGEN_TURNS=0;
		PETRIFY_TURNS=0;
		clearSeeds();
		clearTrees();
		while (!turnOrder.empty()) {
			turnOrder.pop();
		}
	}

	void GameOver() {
		GAME_STATE=GAME_OVER;
		ResetTerminal(STORY_TEXT4+PLAYER_NAME+STORY_TEXT5);
		playMusic(&SONG_GAMEOVER);
		for (int i=0;i<CURRENT_ENCOUNTER.entities.size();i++) {
			CURRENT_ENCOUNTER.entities[i]->hp=CURRENT_ENCOUNTER.entities[i]->startingHP;
			CURRENT_ENCOUNTER.entities[i]->spr=CURRENT_ENCOUNTER.entities[i]->startingspr;
			CURRENT_ENCOUNTER.entities[i]->fixedTurnOrderInd=0;
			CURRENT_ENCOUNTER.entities[i]->hidden=false;
			CURRENT_ENCOUNTER.entities[i]->turnComplete=false;
			CURRENT_ENCOUNTER.entities[i]->speed=0;
		}
		resetBattleState();
		fadeOut();
	}

	void DrawBuffs(vf2d pos) {
		DrawBuffs(pos,NULL);
	}

	void DrawBuffs(vf2d pos,Entity*ent) {
		if (ent!=NULL) {
			vi2d buffPos={0,0};
			if (ent->speed==-1) {
				DrawDecal(pos+buffPos,SLOWED_DECAL);
				buffPos.x+=16;
			} else 
			if (ent->speed==1) {
				DrawDecal(pos+buffPos,SPEED_DECAL);
				buffPos.x+=16;
			}
			if (ent->hidden) {
				DrawDecal(pos+buffPos,HIDDEN_DECAL);
				buffPos.x+=16;
			}
		} else {
			vi2d buffPos={0,0};
			//Draw Player's Buffs.
			if (FOOD_REGEN_TURNS>0) {
				DrawDecal(pos+buffPos,HP_REGEN_DECAL);
				std::string txt=std::to_string(FOOD_REGEN_TURNS);
				DrawStringDecal({pos.x+buffPos.x+16-GetTextSize(txt).x+1,pos.y+16-GetTextSize(txt).y+1},txt,VERY_DARK_GREEN);
				DrawStringDecal({pos.x+buffPos.x+16-GetTextSize(txt).x,pos.y+16-GetTextSize(txt).y},txt);
				buffPos.x+=16;
			}
			if (PETRIFY_TURNS>0) {
				DrawDecal(pos+buffPos,PETRIFY_DECAL);
				std::string txt=std::to_string(PETRIFY_TURNS);
				DrawStringDecal({pos.x+buffPos.x+16-GetTextSize(txt).x+1,pos.y+16-GetTextSize(txt).y+1},txt,VERY_DARK_RED);
				DrawStringDecal({pos.x+buffPos.x+16-GetTextSize(txt).x,pos.y+16-GetTextSize(txt).y},txt);
				buffPos.x+=16;
			}
		}
	}

	void DrawStars(){
		for (int i=0;i<150;i++) {
			float distanceFromCenter=distancetoCoords(starpixels[i]->pos,{WIDTH/2,HEIGHT/2-24});
			FillRectDecal(starpixels[i]->pos,starpixels[i]->size,{(uint8_t)(starpixels[i]->r),(uint8_t)(starpixels[i]->g),(uint8_t)(starpixels[i]->b),(uint8_t)(std::clamp(distanceFromCenter/64.0*255,0.0,255.0))});
		}
	}

	bool UpPressed(){
		return GetKey(W).bPressed||GetKey(UP).bPressed||GetKey(NP8).bPressed||MOUSE_PRESSED_DOWN&&GetMouseY()<=HEIGHT-128+32&&GetMouseY()>=HEIGHT-128&&GetMouseX()<=HEIGHT-128;
	}
	bool DownPressed(){
		return GetKey(S).bPressed||GetKey(DOWN).bPressed||GetKey(NP5).bPressed||GetKey(NP2).bPressed||MOUSE_PRESSED_DOWN&&GetMouseY()<=HEIGHT&&GetMouseY()>=HEIGHT-32&&GetMouseX()<=HEIGHT-128;
	}
	bool LeftPressed(){
		return GetKey(A).bPressed||GetKey(LEFT).bPressed||GetKey(NP4).bPressed||MOUSE_PRESSED_DOWN&&GetMouseX()<=32&&GetMouseX()>=0&&GetMouseY()>=HEIGHT-128;
	}
	bool RightPressed(){
		return GetKey(D).bPressed||GetKey(RIGHT).bPressed||GetKey(NP6).bPressed||MOUSE_PRESSED_DOWN&&GetMouseX()<=128&&GetMouseX()>=96&&GetMouseY()>=HEIGHT-128;
	}
	bool UpHeld(){
		return GetKey(W).bHeld||GetKey(UP).bHeld||GetKey(NP8).bHeld||MOUSE_DOWN&&GetMouseY()<=HEIGHT-128+32&&GetMouseY()>=HEIGHT-128&&GetMouseX()<=HEIGHT-128;
	}
	bool DownHeld(){
		return GetKey(S).bHeld||GetKey(DOWN).bHeld||GetKey(NP5).bHeld||GetKey(NP2).bHeld||MOUSE_DOWN&&GetMouseY()<=HEIGHT&&GetMouseY()>=HEIGHT-32&&GetMouseX()<=HEIGHT-128;
	}
	bool LeftHeld(){
		return GetKey(A).bHeld||GetKey(LEFT).bHeld||GetKey(NP4).bHeld||MOUSE_DOWN&&GetMouseX()<=32&&GetMouseX()>=0&&GetMouseY()>=HEIGHT-128;
	}
	bool RightHeld(){
		return GetKey(D).bHeld||GetKey(RIGHT).bHeld||GetKey(NP6).bHeld||MOUSE_DOWN&&GetMouseX()<=128&&GetMouseX()>=96&&GetMouseY()>=HEIGHT-128;
	}
	bool UpReleased(){
		return GetKey(W).bReleased||GetKey(UP).bReleased||GetKey(NP8).bReleased||MOUSE_RELEASED&&GetMouseY()<=HEIGHT-128+32&&GetMouseY()>=HEIGHT-128&&GetMouseX()<=HEIGHT-128;
	}
	bool DownReleased(){
		return GetKey(S).bReleased||GetKey(DOWN).bReleased||GetKey(NP5).bReleased||GetKey(NP2).bReleased||MOUSE_RELEASED&&GetMouseY()<=HEIGHT&&GetMouseY()>=HEIGHT-32&&GetMouseX()<=HEIGHT-128;
	}
	bool LeftReleased(){
		return GetKey(A).bReleased||GetKey(LEFT).bReleased||GetKey(NP4).bReleased||MOUSE_RELEASED&&GetMouseX()<=32&&GetMouseX()>=0&&GetMouseY()>=HEIGHT-128;
	}
	bool RightReleased(){
		return GetKey(D).bReleased||GetKey(RIGHT).bReleased||GetKey(NP6).bReleased||MOUSE_RELEASED&&GetMouseX()<=128&&GetMouseX()>=96&&GetMouseY()>=HEIGHT-128;
	}
};


int main()
{
	SeasonsOfLoneliness demo;
	if (demo.Construct(256, 224, 4, 4))
		demo.Start();
	return 0;
}
