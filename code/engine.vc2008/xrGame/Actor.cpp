#include "stdafx.h"
#include "Actor_Flags.h"
#include "hudmanager.h"
#ifdef DEBUG
#	include "PHDebug.h"
#endif // DEBUG
#include "alife_space.h"
#include "hit.h"
#include "PHDestroyable.h"
#include "Car.h"
#include "CarWeapon.h"
#include "xrserver_objects_alife_monsters.h"
#include "CameraLook.h"
#include "CameraFirstEye.h"
#include "effectorfall.h"
#include "EffectorBobbing.h"
#include "ActorEffector.h"
#include "EffectorZoomInertion.h"
#include "SleepEffector.h"
#include "character_info.h"
#include "CustomOutfit.h"
#include "actorcondition.h"
#include "UIGameCustom.h"
#include "../xrphysics/matrix_utils.h"
#include "clsid_game.h"
#include "Grenade.h"
#include "Torch.h"

// breakpoints
#include "../xrEngine/xr_input.h"
//
#include "Actor.h"
#include "ActorAnimation.h"
#include "actor_anim_defs.h"
#include "HudItem.h"
#include "../xrScripts/export/ai_sounds.h"
#include "ai_space.h"
#include "trade.h"
#include "inventory.h"
#include "level.h"
#include "GamePersistent.h"
#include "xrmessages.h"
#include "string_table.h"
#include "usablescriptobject.h"
#include "../xrEngine/cl_intersect.h"
#include "alife_registry_wrappers.h"
#include "../Include/xrRender/Kinematics.h"
#include "artefact.h"
#include "CharacterPhysicsSupport.h"
#include "material_manager.h"
#include "../xrphysics/IColisiondamageInfo.h"
#include "ui/UIMainIngameWnd.h"
#include "map_manager.h"
#include "GameTaskManager.h"
#include "actor_memory.h"
#include "Script_Game_Object.h"
#include "Game_Object_Space.h"
#include "script_callback_ex.h"
#include "InventoryBox.h"
#include "location_manager.h"
#include "player_hud.h"
#include "ai/monsters/basemonster/base_monster.h"

#include "../Include/xrRender/UIRender.h"

#include "ai_object_location.h"
#include "ui/uiMotionIcon.h"
#include "ui/UIActorMenu.h"
#include "ActorHelmet.h"
#include "UI/UIDragDropReferenceList.h"
#include "ZoneCampfire.h"

const u32		patch_frames	= 50;
const float		respawn_delay	= 1.f;
const float		respawn_auto	= 7.f;

static float IReceived = 0;
static float ICoincidenced = 0;
extern float cammera_into_collision_shift ;

string32		ACTOR_DEFS::g_quick_use_slots[4]={NULL, NULL, NULL, NULL};
//skeleton

bool isCampFireAt;

static Fbox		bbStandBox;
static Fbox		bbCrouchBox;
static Fvector	vFootCenter;
static Fvector	vFootExt;

Flags32			psActorFlags={AF_GODMODE_RT|AF_AUTOPICKUP|AF_RUN_BACKWARD|AF_IMPORTANT_SAVE};
int				psActorSleepTime = 1;



CActor::CActor() : CEntityAlive(),current_ik_cam_shift(0)
{
	game_news_registry		= xr_new<CGameNewsRegistryWrapper		>();
	// Cameras
	cameras[eacFirstEye] = xr_new<CCameraFirstEye>(this, CCameraBase::flKeepPitch);
    cameras[eacFirstEye]->Load("actor_firsteye_cam");

	cameras[eacLookAt] = xr_new<CCameraLook2>(this);
    cameras[eacLookAt]->Load("actor_look_cam_psp");
 
    cameras[eacFreeLook] = xr_new<CCameraLook>(this);
    cameras[eacFreeLook]->Load("actor_free_cam");

    cam_active = eacFirstEye;
	fPrevCamPos = 0.0f;
	vPrevCamDir.set(0.f,0.f,1.f);
	fCurAVelocity = 0.0f;
	// Раскачка
	pCamBobbing = 0;


	r_torso.yaw				= 0;
	r_torso.pitch			= 0;
	r_torso.roll			= 0;
	r_torso_tgt_roll		= 0;
	r_model_yaw				= 0;
	r_model_yaw_delta		= 0;
	r_model_yaw_dest		= 0;

	b_DropActivated			= 0;
	f_DropPower				= 0.f;

	m_fRunFactor			= 2.f;
	m_fCrouchFactor			= 0.2f;
	m_fClimbFactor			= 1.f;
	m_fCamHeightFactor		= 0.87f;

	m_fFallTime				=	s_fFallTime;
	m_bAnimTorsoPlayed		=	false;

	m_pPhysicsShell			=	NULL;

	m_fFeelGrenadeRadius	=	10.0f;
	m_fFeelGrenadeTime      =	1.0f;
	
	m_holder				=	NULL;
	m_holderID				=	u16(-1);

	m_CapmfireWeLookingAt	= nullptr;
#ifdef DEBUG
	Device.seqRender.Add	(this,REG_PRIORITY_LOW);
#endif

	//��������� ������������� ����� � inventory
	inventory().SetBeltUseful(true);

	m_pPersonWeLookingAt	= 0;
	m_pVehicleWeLookingAt	= 0;
	m_pObjectWeLookingAt	= 0;
	pStatGraph				= 0;
	m_pActorEffector		= 0;
	m_vehicle_anims	        = xr_new<SActorVehicleAnims>();
	m_entity_condition		= 0;
	m_statistic_manager		= 0;
	m_sDefaultObjAction		= 0;
	m_pUsableObject			= 0;
	SetZoomAimingMode		(false);
	m_fSprintFactor			= 4.f;
	m_anims					= xr_new<SActorMotions>();
	m_iLastHitterID			= u16(-1);
	m_iLastHittingWeaponID	= u16(-1);
	m_bPickupMode			= false;
	//-----------------------------------------------------------------------------------
	m_memory				= xr_new<CActorMemory>(this);
	m_bOutBorder			= false;
	m_hit_probability		= 1.f;
	m_feel_touch_characters = 0;
	//-----------------------------------------------------------------------------------
	m_dwILastUpdateTime		= 0;

	m_location_manager		= xr_new<CLocationManager>(this);
	m_block_sprint_counter	= 0;

	m_disabled_hitmarks		= false;
	m_inventory_disabled	= false;
	
	// Alex ADD: for smooth crouch fix
	CurrentHeight = 0.f;
}


CActor::~CActor()
{
	xr_delete				(m_location_manager);
	xr_delete				(m_memory);
	xr_delete				(game_news_registry);
#ifdef DEBUG
	Device.seqRender.Remove(this);
#endif
	//xr_delete(Weapons);
	for (int i=0; i<eacMaxCam; ++i) xr_delete(cameras[i]);

	m_HeavyBreathSnd.destroy();
	m_BloodSnd.destroy		();
	m_DangerSnd.destroy		();

	xr_delete				(m_pActorEffector);
	xr_delete				(m_pPhysics_support);
	xr_delete				(m_anims);
	xr_delete               (m_vehicle_anims);
}

void CActor::reinit	()
{
	character_physics_support()->movement()->CreateCharacter		();
	character_physics_support()->movement()->SetPhysicsRefObject	(this);
	CEntityAlive::reinit						();
	CInventoryOwner::reinit						();

	character_physics_support()->in_Init		();
	material().reinit							();

	m_pUsableObject								= NULL;
	memory().reinit							();
	
	set_input_external_handler					(0);
	m_time_lock_accel							= 0;
}

void CActor::reload	(LPCSTR section)
{
	CEntityAlive::reload		(section);
	CInventoryOwner::reload		(section);
	material().reload			(section);
	CStepManager::reload		(section);
	memory().reload			(section);
	m_location_manager->reload	(section);
}
void set_box(LPCSTR section, CPHMovementControl &mc, u32 box_num )
{
	Fbox	bb;Fvector	vBOX_center,vBOX_size;
	// m_PhysicMovementControl: BOX
	string64 buff, buff1;
	strconcat( sizeof(buff), buff, "ph_box",itoa( box_num, buff1, 10 ),"_center" );
	vBOX_center= pSettings->r_fvector3	(section, buff	);
	strconcat( sizeof(buff), buff, "ph_box",itoa( box_num, buff1, 10 ),"_size" );
	vBOX_size	= pSettings->r_fvector3	(section, buff);
	vBOX_size.y += cammera_into_collision_shift/2.f;
	bb.set	(vBOX_center,vBOX_center); bb.grow(vBOX_size);
	mc.SetBox		(box_num,bb);
}
void CActor::Load	(LPCSTR section )
{
	// Msg						("Loading actor: %s",section);
	inherited::Load				(section);
	material().Load				(section);
	CInventoryOwner::Load		(section);
	m_location_manager->Load	(section);

	OnDifficultyChanged		();
	//////////////////////////////////////////////////////////////////////////
	ISpatial*		self			=	smart_cast<ISpatial*> (this);
	if (self)	{
		self->spatial.type	|=	STYPE_VISIBLEFORAI;
		self->spatial.type	&= ~STYPE_REACTTOSOUND;
	}
	//////////////////////////////////////////////////////////////////////////
	// m_PhysicMovementControl: Crash speed and mass
	float	cs_min		= pSettings->r_float	(section,"ph_crash_speed_min"	);
	float	cs_max		= pSettings->r_float	(section,"ph_crash_speed_max"	);
	float	mass		= pSettings->r_float	(section,"ph_mass"				);
	character_physics_support()->movement()->SetCrashSpeeds	(cs_min,cs_max);
	character_physics_support()->movement()->SetMass		(mass);
	if(pSettings->line_exist(section,"stalker_restrictor_radius"))
		character_physics_support()->movement()->SetActorRestrictorRadius(rtStalker,pSettings->r_float(section,"stalker_restrictor_radius"));
	if(pSettings->line_exist(section,"stalker_small_restrictor_radius"))
		character_physics_support()->movement()->SetActorRestrictorRadius(rtStalkerSmall,pSettings->r_float(section,"stalker_small_restrictor_radius"));
	if(pSettings->line_exist(section,"medium_monster_restrictor_radius"))
		character_physics_support()->movement()->SetActorRestrictorRadius(rtMonsterMedium,pSettings->r_float(section,"medium_monster_restrictor_radius"));
	character_physics_support()->movement()->Load(section);

	set_box( section, *character_physics_support()->movement(), 2 );
	set_box( section, *character_physics_support()->movement(), 1 );
	set_box( section, *character_physics_support()->movement(), 0 );

	m_fWalkAccel				= pSettings->r_float(section,"walk_accel");	
	m_fJumpSpeed				= pSettings->r_float(section,"jump_speed");
	m_fRunFactor				= pSettings->r_float(section,"run_coef");
	m_fRunBackFactor			= pSettings->r_float(section,"run_back_coef");
	m_fWalkBackFactor			= pSettings->r_float(section,"walk_back_coef");
	m_fCrouchFactor				= pSettings->r_float(section,"crouch_coef");
	m_fClimbFactor				= pSettings->r_float(section,"climb_coef");
	m_fSprintFactor				= pSettings->r_float(section,"sprint_koef");

	m_fWalk_StrafeFactor		= READ_IF_EXISTS(pSettings, r_float, section, "walk_strafe_coef", 1.0f);
	m_fRun_StrafeFactor			= READ_IF_EXISTS(pSettings, r_float, section, "run_strafe_coef", 1.0f);


	m_fCamHeightFactor			= pSettings->r_float(section,"camera_height_factor");
	character_physics_support()->movement()->SetJumpUpVelocity(m_fJumpSpeed);
	float AirControlParam		= pSettings->r_float(section,"air_control_param"	);
	character_physics_support()->movement()->SetAirControlParam(AirControlParam);

	m_fPickupInfoRadius			= pSettings->r_float(section,"pickup_info_radius");

	m_fFeelGrenadeRadius		= pSettings->r_float(section,"feel_grenade_radius");
	m_fFeelGrenadeTime			= pSettings->r_float(section,"feel_grenade_time");
	m_fFeelGrenadeTime			*= 1000.0f;
	
	character_physics_support()->in_Load		(section);
	

	LPCSTR hit_snd_sect = pSettings->r_string(section,"hit_sounds");
	for(int hit_type=0; hit_type<(int)ALife::eHitTypeMax; ++hit_type)
	{
		LPCSTR hit_name = ALife::g_cafHitType2String((ALife::EHitType)hit_type);
		LPCSTR hit_snds = READ_IF_EXISTS(pSettings, r_string, hit_snd_sect, hit_name, "");
		int cnt = _GetItemCount(hit_snds);
		string128		tmp;
		VERIFY			(cnt!=0);
		for(int i=0; i<cnt;++i)
		{
			sndHit[hit_type].push_back		(ref_sound());
			sndHit[hit_type].back().create	(_GetItem(hit_snds,i,tmp),st_Effect,sg_SourceType);
		}
		char buf[256];

		::Sound->create		(sndDie[0],			strconcat(sizeof(buf),buf,*cName(),"\\die0"), st_Effect,SOUND_TYPE_MONSTER_DYING);
		::Sound->create		(sndDie[1],			strconcat(sizeof(buf),buf,*cName(),"\\die1"), st_Effect,SOUND_TYPE_MONSTER_DYING);
		::Sound->create		(sndDie[2],			strconcat(sizeof(buf),buf,*cName(),"\\die2"), st_Effect,SOUND_TYPE_MONSTER_DYING);
		::Sound->create		(sndDie[3],			strconcat(sizeof(buf),buf,*cName(),"\\die3"), st_Effect,SOUND_TYPE_MONSTER_DYING);

		m_HeavyBreathSnd.create	(pSettings->r_string(section,"heavy_breath_snd"), st_Effect,SOUND_TYPE_MONSTER_INJURING);
		m_BloodSnd.create		(pSettings->r_string(section,"heavy_blood_snd"), st_Effect,SOUND_TYPE_MONSTER_INJURING);
		m_DangerSnd.create		(pSettings->r_string(section,"heavy_danger_snd"), st_Effect,SOUND_TYPE_MONSTER_INJURING);
	}
	if( psActorFlags.test(AF_PSP) )
		cam_Set					(eacLookAt);
	else
		cam_Set					(eacFirstEye);

	// sheduler
	shedule.t_min				= shedule.t_max = 1;

	// ��������� ��������� ��������
	m_fDispBase					= pSettings->r_float		(section,"disp_base"		 );
	m_fDispBase					= deg2rad(m_fDispBase);

	m_fDispAim					= pSettings->r_float		(section,"disp_aim"		 );
	m_fDispAim					= deg2rad(m_fDispAim);

	m_fDispVelFactor			= pSettings->r_float		(section,"disp_vel_factor"	 );
	m_fDispAccelFactor			= pSettings->r_float		(section,"disp_accel_factor" );
	m_fDispCrouchFactor			= pSettings->r_float		(section,"disp_crouch_factor");
	m_fDispCrouchNoAccelFactor	= pSettings->r_float		(section,"disp_crouch_no_acc_factor");

	LPCSTR							default_outfit = READ_IF_EXISTS(pSettings,r_string,section,"default_outfit",0);
	SetDefaultVisualOutfit			(default_outfit);

	invincibility_fire_shield_1st	= READ_IF_EXISTS(pSettings,r_string,section,"Invincibility_Shield_1st",0);
	invincibility_fire_shield_3rd	= READ_IF_EXISTS(pSettings,r_string,section,"Invincibility_Shield_3rd",0);
//-----------------------------------------
	m_AutoPickUp_AABB				= READ_IF_EXISTS(pSettings,r_fvector3,section,"AutoPickUp_AABB",Fvector().set(0.02f, 0.02f, 0.02f));
	m_AutoPickUp_AABB_Offset		= READ_IF_EXISTS(pSettings,r_fvector3,section,"AutoPickUp_AABB_offs",Fvector().set(0, 0, 0));

	CStringTable string_table;
	m_sCharacterUseAction			= "character_use";
	m_sDeadCharacterUseAction		= "dead_character_use";
	m_sDeadCharacterUseOrDragAction	= "dead_character_use_or_drag";
	m_sDeadCharacterDontUseAction	= "dead_character_dont_use";
	m_sCarCharacterUseAction		= "car_character_use";
	m_sInventoryItemUseAction		= "inventory_item_use";
	m_sInventoryBoxUseAction		= "inventory_box_use";
	m_sCampfireIgniteAction			= "campfire_ignite";
	m_sCampfireExtinguishAction = "campfire_extinguish";
	//---------------------------------------------------------------------
	m_sHeadShotParticle	= READ_IF_EXISTS(pSettings,r_string,section,"HeadShotParticle",0);
	
	// Alex ADD: for smooth crouch fix
	CurrentHeight = CameraHeight();	
}

void CActor::PHHit(SHit &H)
{
	m_pPhysics_support->in_Hit( H, false );
}

struct playing_pred
{
	IC	bool	operator()			(ref_sound &s)
	{
		return	(NULL != s._feedback() );
	}
};

void	CActor::Hit(SHit* pHDS)
{
	bool b_initiated = pHDS->aim_bullet; // physics strike by poltergeist

	pHDS->aim_bullet = false;

	SHit& HDS = *pHDS;
	if (HDS.hit_type < ALife::eHitTypeBurn || HDS.hit_type >= ALife::eHitTypeMax)
	{
		string256	err;
		xr_sprintf(err, "Unknown/unregistered hit type [%d]", HDS.hit_type);
		R_ASSERT2(0, err);

	}
#ifdef DEBUG
	if (ph_dbg_draw_mask.test(phDbgCharacterControl)) {
		DBG_OpenCashedDraw();
		Fvector to; to.add(Position(), Fvector().mul(HDS.dir, HDS.phys_impulse()));
		DBG_DrawLine(Position(), to, D3DCOLOR_XRGB(124, 124, 0));
		DBG_ClosedCashedDraw(500);
	}
#endif // DEBUG

	bool bPlaySound = true;
	if (!g_Alive()) bPlaySound = false;

	if (!sndHit[HDS.hit_type].empty() &&
		conditions().PlayHitSound(pHDS))
	{
		ref_sound& S = sndHit[HDS.hit_type][Random.randI((u32)sndHit[HDS.hit_type].size())];
		bool b_snd_hit_playing = sndHit[HDS.hit_type].end() != std::find_if(sndHit[HDS.hit_type].begin(), sndHit[HDS.hit_type].end(), playing_pred());

		if (ALife::eHitTypeExplosion == HDS.hit_type)
		{
			if (this == Level().CurrentControlEntity())
			{
				S.set_volume(10.0f);
				if (!m_sndShockEffector) {
					m_sndShockEffector = xr_new<SndShockEffector>();
					m_sndShockEffector->Start(this, float(S.get_length_sec()*1000.0f), HDS.damage());
				}
			}
			else
				bPlaySound = false;
		}
		if (bPlaySound && !b_snd_hit_playing)
		{
			Fvector point = Position();
			point.y += CameraHeight();
			S.play_at_pos(this, point);
		}
	}


	//slow actor, only when he gets hit
	m_hit_slowmo = conditions().HitSlowmo(pHDS);

	//---------------------------------------------------------------
	if ((Level().CurrentViewEntity() == this) &&
		(HDS.hit_type == ALife::eHitTypeFireWound))
	{
		CObject* pLastHitter = Level().Objects.net_Find(m_iLastHitterID);
		CObject* pLastHittingWeapon = Level().Objects.net_Find(m_iLastHittingWeaponID);
		HitSector(pLastHitter, pLastHittingWeapon);
	}

	if ((mstate_real&mcSprint) && Level().CurrentControlEntity() == this && conditions().DisableSprint(pHDS))
	{
		bool const is_special_burn_hit_2_self = (pHDS->who == this) && (pHDS->boneID == BI_NONE) &&
			((pHDS->hit_type == ALife::eHitTypeBurn) || (pHDS->hit_type == ALife::eHitTypeLightBurn));
		if (!is_special_burn_hit_2_self)
			mstate_wishful &= ~mcSprint;
	}
	if (!m_disabled_hitmarks)
	{
		bool b_fireWound = (pHDS->hit_type == ALife::eHitTypeFireWound || pHDS->hit_type == ALife::eHitTypeWound_2);
		b_initiated = b_initiated && (pHDS->hit_type == ALife::eHitTypeStrike);

		if (b_fireWound || b_initiated)
			HitMark(HDS.damage(), HDS.dir, HDS.who, HDS.bone(), HDS.p_in_bone_space, HDS.impulse, HDS.hit_type);
	}

	float hit_power = HitArtefactsOnBelt(HDS.damage(), HDS.hit_type);

	if (GodMode())
	{
		HDS.power = 0.0f;
		inherited::Hit(&HDS);
		return;
	}
	else
	{
		HDS.power = hit_power;
		HDS.add_wound = true;
		inherited::Hit(&HDS);
	}
}

void CActor::HitMark(float P, Fvector dir, CObject* who_object, s16 element, Fvector position_in_bone_space, float impulse, ALife::EHitType hit_type_)
{
	if (g_Alive() && Local() && (Level().CurrentEntity() == this))
	{
		HUD().HitMarked				(0, P, dir);

		CEffectorCam* ce			= Cameras().GetCamEffector((ECamEffectorType)effFireHit);
		if( ce )					return;

		int id						= -1;
		Fvector						cam_pos,cam_dir,cam_norm;
		cam_Active()->Get			(cam_pos,cam_dir,cam_norm);
		cam_dir.normalize_safe		();
		dir.normalize_safe			();

		float ang_diff				= angle_difference	(cam_dir.getH(), dir.getH());
		Fvector						cp;
		cp.crossproduct				(cam_dir,dir);
		bool bUp					=(cp.y>0.0f);

		Fvector cross;
		cross.crossproduct			(cam_dir, dir);
		VERIFY						(ang_diff>=0.0f && ang_diff<=PI);

		float _s1 = PI_DIV_8;
		float _s2 = _s1 + PI_DIV_4;
		float _s3 = _s2 + PI_DIV_4;
		float _s4 = _s3 + PI_DIV_4;

		if ( ang_diff <= _s1 )
		{
			id = 2;
		}
		else if ( ang_diff > _s1 && ang_diff <= _s2 )
		{
			id = (bUp)?5:7;
		}
		else if ( ang_diff > _s2 && ang_diff <= _s3 )
		{
			id = (bUp)?3:1;
		}
		else if ( ang_diff > _s3 && ang_diff <= _s4 )
		{
			id = (bUp)?4:6;
		}
		else if( ang_diff > _s4 )
		{
			id = 0;
		}
		else
		{
			VERIFY(0);
		}

		string64 sect_name;
		xr_sprintf( sect_name, "effector_fire_hit_%d", id );
		AddEffector( this, effFireHit, sect_name, P * 0.001f );

	}//if hit_type
}

void CActor::HitSignal(float perc, Fvector& vLocalDir, CObject* who, s16 element)
{
	if (g_Alive()) 
	{

		// check damage bone
		Fvector D;
		XFORM().transform_dir(D,vLocalDir);

		float	yaw, pitch;
		D.getHP(yaw,pitch);
		IRenderVisual *pV = Visual();
		IKinematicsAnimated *tpKinematics = smart_cast<IKinematicsAnimated*>(pV);
		IKinematics *pK = smart_cast<IKinematics*>(pV);
		VERIFY(tpKinematics);
#pragma todo("Dima to Dima : forward-back bone impulse direction has been determined incorrectly!")
		MotionID motion_ID = m_anims->m_normal.m_damage[iFloor(pK->LL_GetBoneInstance(element).get_param(1) + (angle_difference(r_model_yaw + r_model_yaw_delta,yaw) <= PI_DIV_2 ? 0 : 1))];
		float power_factor = perc/100.f; clamp(power_factor,0.f,1.f);
		VERIFY(motion_ID.valid());
		tpKinematics->PlayFX(motion_ID,power_factor);

		callback(GameObject::eHit)(lua_game_object(), perc, vLocalDir, smart_cast<const CGameObject*>(who)->lua_game_object(), element);
	}
}
void start_tutorial(LPCSTR name);
void CActor::Die(CObject* who)
{
	inherited::Die(who);

	u16 I = inventory().FirstSlot();
	u16 E = inventory().LastSlot();

	for (; I <= E; ++I)
	{
		PIItem item_in_slot = inventory().ItemFromSlot(I);
		if (I == inventory().GetActiveSlot())
		{
			if (item_in_slot)
			{
				CGrenade* grenade = smart_cast<CGrenade*>(item_in_slot);
				if (grenade)
					grenade->DropGrenade();
				else
					item_in_slot->SetDropManual(TRUE);
			};
			continue;
		}
		else
		{
			CCustomOutfit *pOutfit = smart_cast<CCustomOutfit *> (item_in_slot);
			if (pOutfit) continue;
		};
		if (item_in_slot)
			inventory().Ruck(item_in_slot);
	};

	TIItemContainer &l_blist = inventory().m_belt;
	while (!l_blist.empty())
		inventory().Ruck(l_blist.front());

    ::Sound->play_at_pos(sndDie[Random.randI(SND_DIE_COUNT)], this, Position());

    m_HeavyBreathSnd.stop();
    m_BloodSnd.stop();
    m_DangerSnd.stop();

	CurrentGameUI()->HideShownDialogs();
	start_tutorial("game_over");

	mstate_wishful &= ~mcAnyMove;
	mstate_real &= ~mcAnyMove;

	xr_delete(m_sndShockEffector);

	luabind::functor<LPCSTR> lua_function;
	R_ASSERT2(ai().script_engine().functor<LPCSTR>("sk_actor_death.is_killed", lua_function), "Can't call lua function!");

	lua_function();
}

void	CActor::SwitchOutBorder(bool new_border_state)
{
	if(new_border_state)
		callback(GameObject::eExitLevelBorder)(lua_game_object());
	else
		callback(GameObject::eEnterLevelBorder)(lua_game_object());
	m_bOutBorder=new_border_state;
}

void CActor::g_Physics			(Fvector& _accel, float jump, float dt)
{
	// Correct accel
	Fvector		accel;
	accel.set					(_accel);
	m_hit_slowmo				-=	dt;
	if(m_hit_slowmo<0)			m_hit_slowmo = 0.f;

	accel.mul					(1.f-m_hit_slowmo);

	if(g_Alive())
	{
		if(mstate_real&mcClimb&&!cameras[eacFirstEye]->bClampYaw)
				accel.set(0.f,0.f,0.f);
		character_physics_support()->movement()->Calculate			(accel,cameras[cam_active]->vDirection,0,jump,dt,false);
		bool new_border_state=character_physics_support()->movement()->isOutBorder();
		if(m_bOutBorder!=new_border_state && Level().CurrentControlEntity() == this)
		{
			SwitchOutBorder(new_border_state);
		}

		if(!psActorFlags.test(AF_NO_CLIP))
			character_physics_support()->movement()->GetPosition		(Position());
		else
		character_physics_support()->movement()->GetPosition		(Position());

		character_physics_support()->movement()->bSleep				=false;
	}

	if (Local() && g_Alive()) 
	{
		if(character_physics_support()->movement()->gcontact_Was)
			Cameras().AddCamEffector		(xr_new<CEffectorFall> (character_physics_support()->movement()->gcontact_Power));

		if (!fis_zero(character_physics_support()->movement()->gcontact_HealthLost))	
		{
			VERIFY( character_physics_support() );
			VERIFY( character_physics_support()->movement() );
			ICollisionDamageInfo* di=character_physics_support()->movement()->CollisionDamageInfo();
			VERIFY( di );
			bool b_hit_initiated =  di->GetAndResetInitiated();
			Fvector hdir;di->HitDir(hdir);
			SetHitInfo(this, NULL, 0, Fvector().set(0, 0, 0), hdir);
			//				Hit	(m_PhysicMovementControl->gcontact_HealthLost,hdir,di->DamageInitiator(),m_PhysicMovementControl->ContactBone(),di->HitPos(),0.f,ALife::eHitTypeStrike);//s16(6 + 2*::Random.randI(0,2))
			if (Level().CurrentControlEntity() == this)
			{
				
				SHit HDS = SHit(character_physics_support()->movement()->gcontact_HealthLost,
//.								0.0f,
								hdir,
								di->DamageInitiator(),
								character_physics_support()->movement()->ContactBone(),
								di->HitPos(),
								0.f,
								di->HitType(),
								0.0f, 
								b_hit_initiated);
//				Hit(&HDS);

				NET_Packet	l_P;
				HDS.GenHeader(GE_HIT, ID());
				HDS.whoID = di->DamageInitiator()->ID();
				HDS.weaponID = di->DamageInitiator()->ID();
				HDS.Write_Packet(l_P);

				u_EventSend	(l_P);
			}
		}
	}
}
float g_fov = 67.5f;

float CActor::currentFOV()
{
	if (!psHUD_Flags.is(HUD_WEAPON|HUD_WEAPON_RT|HUD_WEAPON_RT2))
		return g_fov;

	CWeapon* pWeapon = smart_cast<CWeapon*>(inventory().ActiveItem());	

	if (pWeapon &&
		pWeapon->IsZoomed() && 
		( !pWeapon->ZoomTexture() || (!pWeapon->IsRotatingToZoom() && pWeapon->ZoomTexture()) )
		 )
	{
		return pWeapon->GetZoomFactor() * (0.75f);
	}else
	{
		return g_fov;
	}
}

static bool bLook_cam_fp_zoom = false;

void CActor::UpdateCL	()
{
	if (g_Alive() && Level().CurrentViewEntity() == this && CurrentGameUI() && NULL == CurrentGameUI()->TopInputReceiver())
 	{
		int dik = get_action_dik(kUSE, 0), dik2 = get_action_dik(kUSE, 1);
		if ((dik && pInput->iGetAsyncKeyState(dik)) || (dik2 && pInput->iGetAsyncKeyState(dik2)))
			m_bPickupMode = true;
 	}

	UpdateInventoryOwner			(Device.dwTimeDelta);

	if(m_feel_touch_characters>0)
	{
		for(xr_vector<CObject*>::iterator it = feel_touch.begin(); it != feel_touch.end(); it++)
		{
			CPhysicsShellHolder	*sh = smart_cast<CPhysicsShellHolder*>(*it);
			if(sh&&sh->character_physics_support())
			{
				sh->character_physics_support()->movement()->UpdateObjectBox(character_physics_support()->movement()->PHCharacter());
			}
		}
	}
	if(m_holder)
		m_holder->UpdateEx( currentFOV() );

	m_snd_noise -= 0.3f*Device.fTimeDelta;

	inherited::UpdateCL				();
	m_pPhysics_support->in_UpdateCL	();


	if (g_Alive()) 
		PickupModeUpdate	();	

	PickupModeUpdate_COD();

	SetZoomAimingMode		(false);
	CWeapon* pWeapon		= smart_cast<CWeapon*>(inventory().ActiveItem());	

	cam_Update(float(Device.dwTimeDelta)/1000.0f, currentFOV());

	if(Level().CurrentEntity() && this->ID()==Level().CurrentEntity()->ID() )
	{
		psHUD_Flags.set( HUD_CROSSHAIR_RT2, true );
		psHUD_Flags.set( HUD_DRAW_RT, true );
	}
	if(pWeapon )
	{
		if(pWeapon->IsZoomed())
		{
			float full_fire_disp = pWeapon->GetFireDispersion(true);

			CEffectorZoomInertion* S = smart_cast<CEffectorZoomInertion*>	(Cameras().GetCamEffector(eCEZoom));
			if(S) 
				S->SetParams(full_fire_disp);

			SetZoomAimingMode		(true);
			//Alun: Force switch to first-person for zooming
			if (!bLook_cam_fp_zoom && cam_active == eacLookAt && cam_Active()->m_look_cam_fp_zoom == true)
			{
				cam_Set(eacFirstEye);
				bLook_cam_fp_zoom = true;
			}			
		}
		else
		{
			//Alun: Switch back to third-person if was forced
			if (bLook_cam_fp_zoom && cam_active == eacFirstEye)
			{
				cam_Set(eacLookAt);
				bLook_cam_fp_zoom = false;
			}
		}			

		if(Level().CurrentEntity() && this->ID()==Level().CurrentEntity()->ID() )
		{
			float fire_disp_full = pWeapon->GetFireDispersion(true, true);
			m_fdisp_controller.SetDispertion(fire_disp_full);
			
			fire_disp_full = m_fdisp_controller.GetCurrentDispertion();

			if (!Device.m_SecondViewport.IsSVPFrame()) //+SecondVP+ Чтобы перекрестие не скакало из за смены FOV (Sin!) [fix for crosshair shaking while SecondVP]
				 HUD().SetCrosshairDisp(fire_disp_full, 0.02f);

			HUD().ShowCrosshair(pWeapon->use_crosshair());
#ifdef DEBUG
			HUD().SetFirstBulletCrosshairDisp(pWeapon->GetFirstBulletDisp());
#endif
			
			BOOL B = ! ((mstate_real & mcLookout) && false);

			psHUD_Flags.set( HUD_WEAPON_RT, B );

			B = B && pWeapon->show_crosshair();

			psHUD_Flags.set( HUD_CROSSHAIR_RT2, B );
			

			
			psHUD_Flags.set( HUD_DRAW_RT,		pWeapon->show_indicators() );
			pWeapon->UpdateSecondVP();
		}

	}
   else
    {
        if (Level().CurrentEntity() && this->ID() == Level().CurrentEntity()->ID())
        {
            HUD().SetCrosshairDisp(0.f);
            HUD().ShowCrosshair(false);
			
			//Alun: Switch back to third-person if was forced
			if (bLook_cam_fp_zoom && cam_active == eacFirstEye)
			{
				cam_Set(eacLookAt);
				bLook_cam_fp_zoom = false;
			}

			Device.m_SecondViewport.SetSVPActive(false);
        }
    }
	float	cs_min		= pSettings->r_float	(cNameSect(),"ph_crash_speed_min"	);
	float	cs_max		= pSettings->r_float	(cNameSect(),"ph_crash_speed_max"	);
	if(psActorFlags.test(AF_GODMODE_RT || AF_GODMODE || AF_NO_CLIP))
	character_physics_support()->movement()->SetCrashSpeeds	(8000,9000);
	else
	character_physics_support()->movement()->SetCrashSpeeds	(cs_min,cs_max);
	
	UpdateDefferedMessages();

	if (g_Alive()) 
		CStepManager::update(this==Level().CurrentViewEntity());

	spatial.type |=STYPE_REACTTOSOUND;

	if(m_sndShockEffector)
	{
		if (this == Level().CurrentViewEntity())
		{
			m_sndShockEffector->Update();

			if(!m_sndShockEffector->InWork() || !g_Alive())
				xr_delete(m_sndShockEffector);
		}
		else
			xr_delete(m_sndShockEffector);
	}
	Fmatrix							trans;

	Cameras().hud_camera_Matrix			(trans);
	
	if(IsFocused())
		g_player_hud->update			(trans);

	m_bPickupMode=false;
}

float	NET_Jump = 0;
void CActor::set_state_box(u32	mstate)
{
		if ( mstate & mcCrouch)
	{
		if (isActorAccelerated(mstate_real, IsZoomAimingMode()))
			character_physics_support()->movement()->ActivateBox(1, true);
		else
			character_physics_support()->movement()->ActivateBox(2, true);
	}
	else 
		character_physics_support()->movement()->ActivateBox(0, true);
}
void CActor::shedule_Update	(u32 DT)
{
	setSVU							(true);

	if(IsFocused())
	{
		if(HUDview())
		{
			CInventoryItem* pInvItem	= inventory().ActiveItem();	
			if( pInvItem )
			{
				CHudItem* pHudItem		= smart_cast<CHudItem*>(pInvItem);	
				if(pHudItem)
				{
					if( pHudItem->IsHidden() )
					{
						g_player_hud->detach_item	(pHudItem);
					}
					else
					{
						g_player_hud->attach_item	(pHudItem);
					}
				}
			}else
			{
					g_player_hud->detach_item_idx	( 0 );
					//Msg("---No active item in inventory(), item 0 detached.");
			}
		}
		else
		{
			g_player_hud->detach_all_items();
			//Msg("---No hud view found, all items detached.");
		}
			
	}

	if(m_holder || !getEnabled() || !Ready())
	{
		m_sDefaultObjAction				= NULL;
		inherited::shedule_Update		(DT);
		return;
	}

	clamp					(DT,0u,100u);
	float	dt	 			=  float(DT)/1000.f;

	// Check controls, create accel, prelimitary setup "mstate_real"
	
	//----------- for E3 -----------------------------
	if (Level().CurrentControlEntity() == this && !Level().IsDemoPlay())
	//------------------------------------------------
	{
		g_cl_CheckControls		(mstate_wishful,NET_SavedAccel,NET_Jump,dt);
		g_cl_Orientate			(mstate_real,dt);
		g_Orientate				(mstate_real,dt);
		g_Physics				(NET_SavedAccel,NET_Jump,dt);
		g_cl_ValidateMState		(dt,mstate_wishful);
		g_SetAnimation			(mstate_real);
		
		// Check for game-contacts
		Fvector C; float R;		
		//m_PhysicMovementControl->GetBoundingSphere	(C,R);
		
		Center( C );
		R = Radius();
		feel_touch_update( C, R );
		Feel_Grenade_Update( m_fFeelGrenadeRadius );

		// Dropping
		if (b_DropActivated)
		{
			f_DropPower			+= dt*0.1f;
			clamp				(f_DropPower,0.f,1.f);
		} else f_DropPower			= 0.f;

		if (!Level().IsDemoPlay())
		{		
		mstate_wishful &=~mcAccel;
		mstate_wishful &=~mcLStrafe;
		mstate_wishful &=~mcRStrafe;
		mstate_wishful &=~mcLLookout;
		mstate_wishful &=~mcRLookout;
		mstate_wishful &=~mcFwd;
		mstate_wishful &=~mcBack;
		if( !psActorFlags.test(AF_CROUCH_TOGGLE) )
			mstate_wishful &=~mcCrouch;
		}
	}
	else 
	{
		make_Interpolation();
		if (NET.size())
		{
			g_sv_Orientate				(mstate_real,dt			);
			g_Orientate					(mstate_real,dt			);
			g_Physics					(NET_SavedAccel,NET_Jump,dt	);			
			if (!m_bInInterpolation)
				g_cl_ValidateMState			(dt,mstate_wishful);
			g_SetAnimation				(mstate_real);
			
			set_state_box(NET_Last.mstate);


		}	
		mstate_old = mstate_real;
	}
	NET_Jump = 0;

	inherited::shedule_Update(DT);
	if (!pCamBobbing)
	{
		pCamBobbing = xr_new<CEffectorBobbing>	();
		Cameras().AddCamEffector			(pCamBobbing);
	}
	pCamBobbing->SetState						(mstate_real, conditions().IsLimping(), IsZoomAimingMode());

	if(this==Level().CurrentControlEntity())
	{
		if(conditions().IsLimping() && g_Alive() && !psActorFlags.test(AF_GODMODE_RT))
		{
			if(!m_HeavyBreathSnd._feedback())	m_HeavyBreathSnd.play_at_pos(this, Fvector().set(0,ACTOR_HEIGHT,0), sm_Looped | sm_2D);
			else								m_HeavyBreathSnd.set_position(Fvector().set(0,ACTOR_HEIGHT,0));
		}
		else if(m_HeavyBreathSnd._feedback())	m_HeavyBreathSnd.stop();
		// -------------------------------
		float bs = conditions().BleedingSpeed();
		if(bs>0.6f)
		{
			Fvector snd_pos;
			snd_pos.set(0,ACTOR_HEIGHT,0);
			if(!m_BloodSnd._feedback())
				m_BloodSnd.play_at_pos(this, snd_pos, sm_Looped | sm_2D);
			else
				m_BloodSnd.set_position(snd_pos);

			float v = bs+0.25f;

			m_BloodSnd.set_volume	(v);
		}else{
			if(m_BloodSnd._feedback())
				m_BloodSnd.stop();
		}

		if(!g_Alive()&&m_BloodSnd._feedback())
				m_BloodSnd.stop();
		// -------------------------------
		bs = conditions().GetZoneDanger();
		if ( bs > 0.1f )
		{
			Fvector snd_pos;
			snd_pos.set(0,ACTOR_HEIGHT,0);
			if(!m_DangerSnd._feedback())
				m_DangerSnd.play_at_pos(this, snd_pos, sm_Looped | sm_2D);
			else
				m_DangerSnd.set_position(snd_pos);

			float v = bs+0.25f;
//			Msg( "bs            = %.2f", bs );

			m_DangerSnd.set_volume	(v);
		}
		else
		{
			if(m_DangerSnd._feedback())
				m_DangerSnd.stop();
		}

		if(!g_Alive()&&m_DangerSnd._feedback())
			m_DangerSnd.stop();
	}
	
	//���� � ������ HUD, �� ���� ������ ������ �� ��������
	if(!character_physics_support()->IsRemoved())
		setVisible				(!HUDview	());

	//��� ����� ����� ����� �����
	collide::rq_result& RQ				= HUD().GetCurrentRayQuery();
	

	float fAcquistionRange = cam_active == eacFirstEye ? 2.0f : 3.0f;
	if (!input_external_handler_installed() && RQ.O && RQ.O->getVisible() && RQ.range < fAcquistionRange)
	{
		m_pObjectWeLookingAt = smart_cast<CGameObject*>(RQ.O);

		CGameObject						*game_object = smart_cast<CGameObject*>(RQ.O);
		m_pUsableObject = smart_cast<CUsableScriptObject*>(game_object);
		m_pInvBoxWeLookingAt = smart_cast<CInventoryBox*>(game_object);
		m_pPersonWeLookingAt = smart_cast<CInventoryOwner*>(game_object);
		m_pVehicleWeLookingAt = smart_cast<CHolderCustom*>(game_object);
		m_CapmfireWeLookingAt = smart_cast<CZoneCampfire*>(game_object);
		CEntityAlive* pEntityAlive = smart_cast<CEntityAlive*>(game_object);

		if (m_pUsableObject && m_pUsableObject->tip_text())
		{
			m_sDefaultObjAction = CStringTable().translate(m_pUsableObject->tip_text());
		}
		else
		{
			if (m_pPersonWeLookingAt && pEntityAlive->g_Alive() && m_pPersonWeLookingAt->IsTalkEnabled())
				m_sDefaultObjAction = m_sCharacterUseAction;
			else if (pEntityAlive && !pEntityAlive->g_Alive())
			{
				if (m_pPersonWeLookingAt && m_pPersonWeLookingAt->deadbody_closed_status())
					m_sDefaultObjAction = m_sDeadCharacterDontUseAction;
				else
				{
					bool b_allow_drag = !!pSettings->line_exist("ph_capture_visuals", pEntityAlive->cNameVisual());
					if (b_allow_drag)
						m_sDefaultObjAction = m_sDeadCharacterUseOrDragAction;
					else if (pEntityAlive->cast_inventory_owner())
						m_sDefaultObjAction = m_sDeadCharacterUseAction;
				} // m_pPersonWeLookingAt
			}
			else if (m_pVehicleWeLookingAt)
				m_sDefaultObjAction = m_sCarCharacterUseAction;
			else if (m_pObjectWeLookingAt && m_pObjectWeLookingAt->cast_inventory_item() && m_pObjectWeLookingAt->cast_inventory_item()->CanTake())
				m_sDefaultObjAction = m_sInventoryItemUseAction;
			else if (m_CapmfireWeLookingAt)
			{
				if (m_CapmfireWeLookingAt->is_on())
				{
					isCampFireAt = true;
					m_sDefaultObjAction = m_sCampfireExtinguishAction;
				}
				else
				{
					isCampFireAt = false;
					m_sDefaultObjAction = m_sCampfireIgniteAction;
				}
			}
			else
				m_sDefaultObjAction = NULL;
		}
	}
	else 
	{
		m_pPersonWeLookingAt	= nullptr;
		m_sDefaultObjAction		= nullptr;
		m_pUsableObject			= nullptr;
		m_pObjectWeLookingAt	= nullptr;
		m_pVehicleWeLookingAt	= nullptr;
		m_pInvBoxWeLookingAt	= nullptr;
		m_CapmfireWeLookingAt	= nullptr;
	}

	UpdateArtefactsOnBeltAndOutfit				();
	m_pPhysics_support->in_shedule_Update		(DT);
};

#include "debug_renderer.h"
void CActor::renderable_Render()
{
	//VERIFY(_valid(XFORM()));
	inherited::renderable_Render();
	bool isHolder = (m_holder && m_holder->allowWeapon() && m_holder->HUDView());
	bool isCam = cam_active == eacFirstEye;

	if ((isCam && ::Render->active_phase() == 1) || !(IsFocused() && isCam && (!m_holder || isHolder)))
		CInventoryOwner::renderable_Render();
}

BOOL CActor::renderable_ShadowGenerate	() 
{
	if(m_holder)
		return FALSE;
	
	return inherited::renderable_ShadowGenerate();
}

void CActor::g_PerformDrop()
{
	b_DropActivated = FALSE;

	PIItem pItem = inventory().ActiveItem();
	if (0 == pItem)			return;

	if (pItem->IsQuestItem()) return;

	u16 s = inventory().GetActiveSlot();
	if (inventory().SlotIsPersistent(s))	return;

	pItem->SetDropManual(TRUE);
}

bool CActor::use_default_throw_force()
{
	if (!g_Alive())
		return false;
	
	return true;
}

float CActor::missile_throw_force()
{
	return 0.f;
}

#ifdef DEBUG
extern	BOOL	g_ShowAnimationInfo		;
#endif // DEBUG
// HUD

void CActor::OnHUDDraw(CCustomHUD*)
{
	R_ASSERT(IsFocused());
	if (!((mstate_real & mcLookout) && false))
		g_player_hud->render_hud();
}

void CActor::RenderIndicator			(Fvector dpos, float r1, float r2, const ui_shader &IndShader)
{
	if (!g_Alive()) return;


	UIRender->StartPrimitive(4, IUIRender::ptTriStrip, IUIRender::pttLIT);

	CBoneInstance& BI = smart_cast<IKinematics*>(Visual())->LL_GetBoneInstance(u16(m_head));
	Fmatrix M;
	smart_cast<IKinematics*>(Visual())->CalculateBones	();
	M.mul						(XFORM(),BI.mTransform);

	Fvector pos = M.c; pos.add(dpos);
	const Fvector& T        = Device.vCameraTop;
	const Fvector& R        = Device.vCameraRight;
	Fvector Vr, Vt;
	Vr.x            = R.x*r1;
	Vr.y            = R.y*r1;
	Vr.z            = R.z*r1;
	Vt.x            = T.x*r2;
	Vt.y            = T.y*r2;
	Vt.z            = T.z*r2;

	Fvector         a,b,c,d;
	a.sub           (Vt,Vr);
	b.add           (Vt,Vr);
	c.invert        (a);
	d.invert        (b);

	UIRender->PushPoint(d.x+pos.x,d.y+pos.y,d.z+pos.z, 0xffffffff, 0.f,1.f);
	UIRender->PushPoint(a.x+pos.x,a.y+pos.y,a.z+pos.z, 0xffffffff, 0.f,0.f);
	UIRender->PushPoint(c.x+pos.x,c.y+pos.y,c.z+pos.z, 0xffffffff, 1.f,1.f);
	UIRender->PushPoint(b.x+pos.x,b.y+pos.y,b.z+pos.z, 0xffffffff, 1.f,0.f);
	UIRender->CacheSetXformWorld(Fidentity);
	UIRender->SetShader(*IndShader);
	UIRender->FlushPrimitive();
};

static float mid_size = 0.097f;
static float fontsize = 15.0f;
static float upsize	= 0.33f;
void CActor::RenderText				(LPCSTR Text, Fvector dpos, float* pdup, u32 color)
{
	if (!g_Alive()) return;
	
	CBoneInstance& BI = smart_cast<IKinematics*>(Visual())->LL_GetBoneInstance(u16(m_head));
	Fmatrix M;
	smart_cast<IKinematics*>(Visual())->CalculateBones	();
	M.mul						(XFORM(),BI.mTransform);
	//------------------------------------------------
	Fvector v0, v1;
	v0.set(M.c); v1.set(M.c);
	Fvector T        = Device.vCameraTop;
	v1.add(T);

	Fvector v0r, v1r;
	Device.mFullTransform.transform(v0r,v0);
	Device.mFullTransform.transform(v1r,v1);
	float size = v1r.distance_to(v0r);
	CGameFont* pFont = UI().Font().pFontArial14;
	if (!pFont) return;
//	float OldFontSize = pFont->GetHeight	();	
	float delta_up = 0.0f;
	if (size < mid_size) delta_up = upsize;
	else delta_up = upsize*(mid_size/size);
	dpos.y += delta_up;
	if (size > mid_size) size = mid_size;
//	float NewFontSize = size/mid_size * fontsize;
	//------------------------------------------------
	M.c.y += dpos.y;

	Fvector4 v_res;	
	Device.mFullTransform.transform(v_res,M.c);

	if (v_res.z < 0 || v_res.w < 0)	return;
	if (v_res.x < -1.f || v_res.x > 1.f || v_res.y<-1.f || v_res.y>1.f) return;

	float x = (1.f + v_res.x)/2.f * (Device.dwWidth);
	float y = (1.f - v_res.y)/2.f * (Device.dwHeight);

	pFont->SetAligment	(CGameFont::alCenter);
	pFont->SetColor		(color);
//	pFont->SetHeight	(NewFontSize);
	pFont->Out			(x,y,Text);
	//-------------------------------------------------
//	pFont->SetHeight(OldFontSize);
	*pdup = delta_up;
};

void CActor::SetPhPosition(const Fmatrix &transform)
{
	if(!m_pPhysicsShell){ 
		character_physics_support()->movement()->SetPosition(transform.c);
	}
	//else m_phSkeleton->S
}

void CActor::ForceTransform(const Fmatrix& m)
{
	character_physics_support()->ForceTransform( m );
	const float block_damage_time_seconds = 2.f;
}

ENGINE_API extern float		psHUD_FOV;
float CActor::Radius()const
{ 
	float R		= inherited::Radius();
	CWeapon* W	= smart_cast<CWeapon*>(inventory().ActiveItem());
	if (W) R	+= W->Radius();
	//	if (HUDview()) R *= 1.f/psHUD_FOV;
	return R;
}

bool		CActor::use_bolts				() const
{
	return CInventoryOwner::use_bolts();
};

int		g_iCorpseRemove = 1;

bool  CActor::NeedToDestroyObject() const
{
		return false;
}

ALife::_TIME_ID	 CActor::TimePassedAfterDeath()	const
{
	if(!g_Alive())
		return Level().timeServer() - GetLevelDeathTime();
	else
		return 0;
}


void CActor::OnItemTake(CInventoryItem *inventory_item)
{
	CInventoryOwner::OnItemTake(inventory_item);
}

void CActor::OnItemDrop(CInventoryItem *inventory_item, bool just_before_destroy)
{
	CInventoryOwner::OnItemDrop(inventory_item, just_before_destroy);

	CCustomOutfit* outfit = smart_cast<CCustomOutfit*>(inventory_item);
	if (outfit && inventory_item->m_ItemCurrPlace.type == eItemPlaceSlot)
	{
		outfit->ApplySkinModel(this, false, false);
	}

	CWeapon* weapon = smart_cast<CWeapon*>(inventory_item);
	if (weapon && inventory_item->m_ItemCurrPlace.type == eItemPlaceSlot)
	{
		weapon->OnZoomOut();
		if (weapon->GetRememberActorNVisnStatus())
			weapon->EnableActorNVisnAfterZoom();
	}

	if (!just_before_destroy && inventory_item->BaseSlot() == GRENADE_SLOT && !inventory().ItemFromSlot(GRENADE_SLOT))
	{
		PIItem grenade = inventory().SameSlot(GRENADE_SLOT, inventory_item, true);

		if (grenade)
			inventory().Slot(GRENADE_SLOT, grenade, true, true);
	}
}


void CActor::OnItemDropUpdate()
{
	CInventoryOwner::OnItemDropUpdate();

	for (auto it : inventory().m_all)
	{
		if (!it->IsInvalid() && !attached(it))
			attach(it);
	}
}


void CActor::OnItemRuck		(CInventoryItem *inventory_item, const SInvItemPlace& previous_place)
{
	CInventoryOwner::OnItemRuck(inventory_item, previous_place);
}

void CActor::OnItemBelt		(CInventoryItem *inventory_item, const SInvItemPlace& previous_place)
{
	CInventoryOwner::OnItemBelt(inventory_item, previous_place);
}

#define ARTEFACTS_UPDATE_TIME 0.100f

void CActor::UpdateArtefactsOnBeltAndOutfit()
{
	static float update_time = 0;
	float f_update_time = 0;

	if (update_time < ARTEFACTS_UPDATE_TIME)
	{
		update_time += conditions().fdelta_time();
		return;
	}
	else
	{
		f_update_time = update_time;
		update_time = 0.0f;
	}

	for (auto it : inventory().m_belt)
	{
		CArtefact*	artefact = smart_cast<CArtefact*>(it);
		if (artefact)
		{
			conditions().ChangeBleeding(artefact->m_fBleedingRestoreSpeed  * f_update_time);
			conditions().ChangeHealth(artefact->m_fHealthRestoreSpeed    * f_update_time);
			conditions().ChangePower(artefact->m_fPowerRestoreSpeed     * f_update_time);
			conditions().ChangeSatiety(artefact->m_fSatietyRestoreSpeed   * f_update_time);
			if (artefact->m_fRadiationRestoreSpeed > 0.0f)
			{
				float val = artefact->m_fRadiationRestoreSpeed - conditions().GetBoostRadiationImmunity();
				clamp(val, 0.0f, val);
				conditions().ChangeRadiation(val * f_update_time);
			}
			else
				conditions().ChangeRadiation(artefact->m_fRadiationRestoreSpeed	* f_update_time);
		}
	}
	CCustomOutfit* outfit = GetOutfit();
	if (outfit)
	{
		conditions().ChangeBleeding(outfit->m_fBleedingRestoreSpeed  * f_update_time);
		conditions().ChangeHealth(outfit->m_fHealthRestoreSpeed    * f_update_time);
		conditions().ChangePower(outfit->m_fPowerRestoreSpeed     * f_update_time);
		conditions().ChangeSatiety(outfit->m_fSatietyRestoreSpeed   * f_update_time);
		conditions().ChangeRadiation(outfit->m_fRadiationRestoreSpeed * f_update_time);
	}
	else
	{
		CHelmet* pHelmet = smart_cast<CHelmet*>(inventory().ItemFromSlot(HELMET_SLOT));
		if (!pHelmet)
		{
			CTorch* pTorch = smart_cast<CTorch*>(inventory().ItemFromSlot(TORCH_SLOT));
			if (pTorch && pTorch->GetNightVisionStatus())
			{
				pTorch->SwitchNightVision(false);
			}
		}
	}
}

float	CActor::HitArtefactsOnBelt(float hit_power, ALife::EHitType hit_type)
{
	for(auto it : inventory().m_belt)
	{
		CArtefact*	artefact = smart_cast<CArtefact*>(it);
		if (artefact)
		{
			hit_power -= artefact->m_ArtefactHitImmunities.AffectHit(1.0f, hit_type);
		}
	}
	clamp(hit_power, 0.0f, flt_max);

	return hit_power;
}

float CActor::GetProtection_ArtefactsOnBelt(ALife::EHitType hit_type)
{
	float sum = 0.0f;
	for (auto it : inventory().m_belt)
	{
		CArtefact*	artefact = smart_cast<CArtefact*>(it);
		if (artefact)
		{
			sum += artefact->m_ArtefactHitImmunities.AffectHit(1.0f, hit_type);
		}
	}
	return sum;
}

void	CActor::SetZoomRndSeed		(s32 Seed)
{
	if (0 != Seed) m_ZoomRndSeed = Seed;
	else m_ZoomRndSeed = s32(Level().timeServer_Async());
};

void	CActor::SetShotRndSeed		(s32 Seed)
{
	if (0 != Seed) m_ShotRndSeed = Seed;
	else m_ShotRndSeed = s32(Level().timeServer_Async());
};

void CActor::spawn_supplies			()
{
	inherited::spawn_supplies		();
	CInventoryOwner::spawn_supplies	();
}


void CActor::AnimTorsoPlayCallBack(CBlend* B)
{
	CActor* actor		= (CActor*)B->CallbackParam;
	actor->m_bAnimTorsoPlayed = FALSE;
}

CPHDestroyable*	CActor::ph_destroyable	()
{
	return smart_cast<CPHDestroyable*>(character_physics_support());
}

CEntityConditionSimple *CActor::create_entity_condition	(CEntityConditionSimple* parentEntCond)
{
	//return (inherited::create_entity_condition((!ec) ? xr_new<CActorCondition>(this) : smart_cast<CActorCondition*>(ec)));
    if (!parentEntCond)
    {
        m_entity_condition = xr_new<CActorCondition>(this);
    }
    else
    {
        m_entity_condition = smart_cast<CActorCondition*>(parentEntCond);
    }

    return		(inherited::create_entity_condition(m_entity_condition));
}

DLL_Pure *CActor::_construct()
{
	m_pPhysics_support = xr_new<CCharacterPhysicsSupport>(CCharacterPhysicsSupport::etActor, this);
	CEntityAlive::_construct();
	CInventoryOwner::_construct();
	CStepManager::_construct();

	return							(this);
}

bool CActor::use_center_to_aim() const
{
	return !!(mstate_real&mcCrouch);
}

bool CActor::can_attach(const CInventoryItem *inventory_item) const
{
	const CAttachableItem	*item = smart_cast<const CAttachableItem*>(inventory_item);
	if (!item ||  !item->can_be_attached())
		return			(false);

	if( m_attach_item_sections.end() == std::find(m_attach_item_sections.begin(),m_attach_item_sections.end(),inventory_item->object().cNameSect()) )
		return false;

	if(attached(inventory_item->object().cNameSect()))
		return false;

	return true;
}

void CActor::OnDifficultyChanged	()
{
	// immunities
	VERIFY(g_SingleGameDifficulty>=egdNovice && g_SingleGameDifficulty<=egdMaster); 
	LPCSTR diff_name				= get_token_name(difficulty_type_token, g_SingleGameDifficulty);
	string128						tmp;
	strconcat						(sizeof(tmp),tmp,"actor_immunities_",diff_name);
	conditions().LoadImmunities		(tmp,pSettings);
	// hit probability
	strconcat						(sizeof(tmp),tmp,"hit_probability_",diff_name);
	m_hit_probability				= pSettings->r_float(*cNameSect(),tmp);
	// two hits death parameters
	strconcat						(sizeof(tmp),tmp,"actor_thd_",diff_name);
	conditions().LoadTwoHitsDeathParams(tmp);
}

CVisualMemoryManager *CActor::visual_memory() const
{
	return (&memory().visual());
}

float CActor::GetMass()
{
	return g_Alive()?character_physics_support()->movement()->GetMass():m_pPhysicsShell?m_pPhysicsShell->getMass():0; 
}

bool CActor::is_on_ground()
{
	return (character_physics_support()->movement()->Environment() != CPHMovementControl::peInAir);
}


bool CActor::is_ai_obstacle() const
{
	return (false);//true);
}

float CActor::GetRestoreSpeed(ALife::EConditionRestoreType const& type)
{
	float res = 0.0f;
	switch (type)
	{
	case ALife::eHealthRestoreSpeed:
	{
		res = conditions().change_v().m_fV_HealthRestore;
		res += conditions().V_SatietyHealth() * ((conditions().GetSatiety() > 0.0f) ? 1.0f : -1.0f);
		for (PIItem &it : inventory().m_belt)
		{
			CArtefact*	artefact = smart_cast<CArtefact*>(it);
			if (artefact)
			{
				res += artefact->m_fHealthRestoreSpeed;
			}
		}
		CCustomOutfit* outfit = GetOutfit();
		if (outfit)
		{
			res += outfit->m_fHealthRestoreSpeed;
		}
		break;
	}
	case ALife::eRadiationRestoreSpeed:
	{
		for (PIItem &it : inventory().m_belt)
		{
			CArtefact*	artefact = smart_cast<CArtefact*>(it);
			if (artefact)
			{
				res += artefact->m_fRadiationRestoreSpeed;
			}
		}
		CCustomOutfit* outfit = GetOutfit();
		if (outfit)
		{
			res += outfit->m_fRadiationRestoreSpeed;
		}
		break;
	}
	case ALife::eSatietyRestoreSpeed:
	{
		res = conditions().V_Satiety();

		for (PIItem &it : inventory().m_belt)
		{
			CArtefact*	artefact = smart_cast<CArtefact*>(it);
			if (artefact)
			{
				res += artefact->m_fSatietyRestoreSpeed;
			}
		}
		CCustomOutfit* outfit = GetOutfit();
		if (outfit)
		{
			res += outfit->m_fSatietyRestoreSpeed;
		}
		break;
	}
	case ALife::ePowerRestoreSpeed:
	{
		res = conditions().GetSatietyPower();

		for (PIItem &it : inventory().m_belt)
		{
			CArtefact*	artefact = smart_cast<CArtefact*>(it);
			if (artefact)
			{
				res += artefact->m_fPowerRestoreSpeed;
			}
		}
		CCustomOutfit* outfit = GetOutfit();
		if (outfit)
		{
			res += outfit->m_fPowerRestoreSpeed;
			VERIFY(outfit->m_fPowerLoss != 0.0f);
			res /= outfit->m_fPowerLoss;
		}
		else
			res /= 0.5f;
		break;
	}
	case ALife::eBleedingRestoreSpeed:
	{
		res = conditions().change_v().m_fV_WoundIncarnation;

		for (PIItem &it : inventory().m_belt)
		{
			CArtefact*	artefact = smart_cast<CArtefact*>(it);
			if (artefact)
			{
				res += artefact->m_fBleedingRestoreSpeed;
			}
		}
		CCustomOutfit* outfit = GetOutfit();
		if (outfit)
		{
			res += outfit->m_fBleedingRestoreSpeed;
		}
		break;
	}
	}//switch

	return res;
}

void CActor::On_SetEntity()
{
	CCustomOutfit* pOutfit = GetOutfit();
	if( !pOutfit )
		g_player_hud->load_default();
	else
		pOutfit->ApplySkinModel(this, true, true);
}

bool CActor::unlimited_ammo()
{
	return !!psActorFlags.test(AF_UNLIMITEDAMMO);
}

#include "level_changer.h"
using namespace luabind;

#pragma optimize("s",on)
void CActor::script_register(lua_State *L)
{
	module(L)
		[
			class_<CActor, CGameObject>("CActor")
			.def(constructor<>()),

		class_<CLevelChanger, CGameObject>("CLevelChanger")
		.def(constructor<>())
		];
}
