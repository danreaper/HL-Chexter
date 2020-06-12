#include "extdll.h"
#include "decals.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "soundent.h"
#include "shake.h"
#include "gamerules.h"

#define WEAPON_BLASTER 16
#define BLASTER_SLOT 4
#define BLASTER_POSITION 4
#define BLASTER_WEIGHT 25

#define BLASTER_DAMAGE 32
#define BLASTER_BEAM_RED 255
#define BLASTER_BEAM_GREEN 0
#define BLASTER_BEAM_BLUE 0
#define BLASTER_BEAM_WIDTH 4
#define BLASTER_BEAM_SPRITE "sprites/smoke.spr"
#define BLASTER_BEAM_BRIGHTNESS 255
#define BLASTER_RANGE 8192
#define BLASTER_OFFSET_FORWARD 16
#define BLASTER_OFFSET_RIGHT 8
#define BLASTER_OFFSET_UP -8

#define BLASTER_MODEL_1STPERSON "models/v_zorcher.mdl"
#define BLASTER_MODEL_3RDPERSON "models/p_9mmhandgun.mdl"
#define BLASTER_MODEL_WORLD "models/w_blaster.mdl"
#define BLASTER_SOUND_SHOOT "weapons/electro5.wav"
#define BLASTER_SOUND_VOLUME 0.25
#define BLASTER_FIRE_DELAY 0.5 // For comparison, glock's is 0.2
#define BLASTER_DEFAULT_AMMO 25
#define BLASTER_MAX_AMMO 100

enum Blaster_Animations
{
	BLASTER_ANIM_IDLE1 = 0,
	BLASTER_ANIM_IDLE2,
	BLASTER_ANIM_IDLE3,
	BLASTER_ANIM_SHOOT,
	BLASTER_ANIM_SHOOT_EMPTY,
	BLASTER_ANIM_RELOAD,
	BLASTER_ANIM_RELOAD_NOT_EMPTY,
	BLASTER_ANIM_DRAW,
	BLASTER_ANIM_HOLSTER,
	BLASTER_ANIM_ADD_SILENCER
};

class CBlaster : public CBasePlayerWeapon
{
public:
	void Spawn();
	void Precache();
	int iItemSlot();
	int GetItemInfo(ItemInfo*);
	BOOL Deploy();
	void PrimaryAttack();
	void WeaponIdle();
	int BeamSprite;
};
LINK_ENTITY_TO_CLASS(weapon_blaster, CBlaster);

void CBlaster::Spawn() {
	pev->classname = MAKE_STRING("weapon_blaster");
	Precache();

	m_iId = WEAPON_BLASTER;
	SET_MODEL(ENT(pev), BLASTER_MODEL_WORLD);

	m_iDefaultAmmo = BLASTER_DEFAULT_AMMO;

	FallInit();
}

void CBlaster::Precache()
{
	PRECACHE_MODEL(BLASTER_MODEL_1STPERSON);
	PRECACHE_MODEL(BLASTER_MODEL_3RDPERSON);
	PRECACHE_MODEL(BLASTER_MODEL_WORLD);
	PRECACHE_SOUND(BLASTER_SOUND_SHOOT);
	BeamSprite = PRECACHE_MODEL(BLASTER_BEAM_SPRITE);
}

int CBlaster::iItemSlot()
{
	return BLASTER_SLOT;
}

int CBlaster::GetItemInfo(ItemInfo* Info) {
	Info->pszName = STRING(pev->classname);
	Info->pszAmmo1 = "uranium";
	Info->iMaxAmmo1 = BLASTER_MAX_AMMO;
	Info->pszAmmo2 = NULL;
	Info->iMaxAmmo2 = -1;
	Info->iMaxClip = 1;
	Info->iSlot = BLASTER_SLOT - 1; // Don't forget the "- 1"
	Info->iPosition = BLASTER_POSITION;
	Info->iFlags = 0;
	Info->iId = WEAPON_BLASTER;
	Info->iWeight = BLASTER_WEIGHT;

	return 1;
}

BOOL CBlaster::Deploy()
{
	return DefaultDeploy(BLASTER_MODEL_1STPERSON,
		BLASTER_MODEL_3RDPERSON,
		BLASTER_ANIM_DRAW,
		"onehanded");
}

void CBlaster::PrimaryAttack(){
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] < 1) return;
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);
	SendWeaponAnim(BLASTER_ANIM_SHOOT);

	UTIL_MakeVectors(m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle);
	Vector AimingDir = gpGlobals->v_forward;
	Vector GunPosition = m_pPlayer->GetGunPosition();
	GunPosition = GunPosition + gpGlobals->v_forward * BLASTER_OFFSET_FORWARD;
	GunPosition = GunPosition + gpGlobals->v_right * BLASTER_OFFSET_RIGHT;
	GunPosition = GunPosition + gpGlobals->v_up * BLASTER_OFFSET_UP;
	Vector EndPoint = GunPosition + AimingDir * BLASTER_RANGE;

	TraceResult TResult;
	edict_t* EntityToIgnore;
	EntityToIgnore = ENT(m_pPlayer->pev);

	UTIL_TraceLine(GunPosition,
		EndPoint,
		dont_ignore_monsters,
		EntityToIgnore,
		&TResult);
	if (TResult.fAllSolid) return;

	CBaseEntity* Victim = CBaseEntity::Instance(TResult.pHit);
	if (Victim != NULL) if (Victim->pev->takedamage)
	{
		ClearMultiDamage();
		Victim->TraceAttack(m_pPlayer->pev, // Person who shot Victim
			BLASTER_DAMAGE, // Amount of damage
			AimingDir,
			&TResult,
			DMG_BULLET); // Damage type.
	}
	MESSAGE_BEGIN(MSG_PAS, SVC_TEMPENTITY, TResult.vecEndPos);

	WRITE_BYTE(TE_BEAMENTPOINT);
	WRITE_SHORT(m_pPlayer->entindex());
	WRITE_COORD(TResult.vecEndPos.x);
	WRITE_COORD(TResult.vecEndPos.y);
	WRITE_COORD(TResult.vecEndPos.z);
	WRITE_SHORT(BeamSprite); // Beam sprite index.
	WRITE_BYTE(0); // Starting frame
	WRITE_BYTE(0); // Framerate
	WRITE_BYTE(1); // How long the beam stays on.
	WRITE_BYTE(BLASTER_BEAM_WIDTH);
	WRITE_BYTE(0); // Noise
	WRITE_BYTE(BLASTER_BEAM_RED);
	WRITE_BYTE(BLASTER_BEAM_GREEN);
	WRITE_BYTE(BLASTER_BEAM_BLUE);
	WRITE_BYTE(BLASTER_BEAM_BRIGHTNESS);
	WRITE_BYTE(0); // Speed, sort of.

	MESSAGE_END();

	EMIT_SOUND_DYN(ENT(m_pPlayer->pev), // Entity sound originates from
		CHAN_WEAPON, // Channel
		BLASTER_SOUND_SHOOT, // Sound to play
		BLASTER_SOUND_VOLUME, // Volume (0.0 to 1.0)
		ATTN_NORM, // Attenuation, whatever the hell that is
		0, // Flags
		150); // Pitch
	m_flNextPrimaryAttack = gpGlobals->time + BLASTER_FIRE_DELAY;
	m_flTimeWeaponIdle = gpGlobals->time + BLASTER_FIRE_DELAY;

	m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] --; 

}

void CBlaster::WeaponIdle()
{
	if (m_flTimeWeaponIdle > gpGlobals->time)
		return;

	m_flTimeWeaponIdle = gpGlobals->time + 3.0;
	SendWeaponAnim(BLASTER_ANIM_IDLE1);
}