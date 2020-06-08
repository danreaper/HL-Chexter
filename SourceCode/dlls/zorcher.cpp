#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "hornet.h"
#include "soundent.h"	// FIXED
#include "gamerules.h"

enum zrch_e {
		ZRCH_IDLE1,
		ZRCH_FIRE,
		ZRCH_DRAW,
		ZRCH_HOLSTER,
};

class CZorcher : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache(void);
	int iItemSlot(void) { return 3; }
	int GetItemInfo(ItemInfo *p);

	void PrimaryAttack( void );
	void SecondaryAttack(void);
	BOOL Deploy(void);
	void WeaponIdle(void);

	float m_flTimeWeaponIdle;
};


LINK_ENTITY_TO_CLASS(weapon_zorcher, CZorcher);

class CZorch : public CBasePlayerAmmo
{
	void Spawn(void)
	{
		Precache();
		SET_MODEL(ENT(pev), "models/w_isotopebox.mdl");
		CBasePlayerAmmo::Spawn();
	}
	void Precache(void)
	{
		PRECACHE_MODEL("models/w_isotopebox.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo(CBaseEntity *pOther)
	{
		if (pOther->GiveAmmo(AMMO_ZORCHER_GIVE, "Zorch", ZORCHER_MAX_CARRY) != -1)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS(ammo_frogs, CZorch);

void CZorcher::Spawn(){
	Precache();
	m_iId = WEAPON_ZORCHER;
	SET_MODEL(ENT(pev), "models/w_egon.mdl");
	m_iDefaultAmmo = ZORCHER_DEFAULT_GIVE;
	FallInit();
}

void CZorcher::Precache(void)
{
	PRECACHE_MODEL("models/v_egon.mdl");
	PRECACHE_MODEL("models/w_egon.mdl");
	PRECACHE_MODEL("models/p_hgun.dml");
	UTIL_PrecacheOther("ammo_zorch");
}

int CZorcher::GetItemInfo(ItemInfo *p) 
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "zorch";
	p->iMaxAmmo1 = ZORCHER_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iSlot = 1;
	p->iPosition = 2;
	p->iId = m_iId = WEAPON_ZORCHER;
	p->iFlags = 0;
	p->iWeight = ZORCHER_WEIGHT;

	return 1;
}

void CZorcher::PrimaryAttack(void)
{
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] == 0)
	{
		PlayEmptySound();
		return;
	}

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = m_pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES);
	Vector vecDir;


	m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;

	SendWeaponAnim(ZRCH_FIRE);
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	UTIL_MakeVectors(m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle);

	int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

#ifdef CLIENT_DLL
	if (!bIsMultiplayer())
#else
	if (!g_pGameRules->IsMultiplayer())
#endif
	{
		// optimized multiplayer. Widened to make it easier to hit a moving player
		vecDir = m_pPlayer->FireBulletsPlayer(1, vecSrc, vecAiming, VECTOR_CONE_6DEGREES, 8192, BULLET_PLAYER_MP5, 2, 0, m_pPlayer->pev, m_pPlayer->random_seed);
	}
	else
	{
		// single player spread
		vecDir = m_pPlayer->FireBulletsPlayer(1, vecSrc, vecAiming, VECTOR_CONE_3DEGREES, 8192, BULLET_PLAYER_MP5, 2, 0, m_pPlayer->pev, m_pPlayer->random_seed);
	}


	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flNextPrimaryAttack = GetNextAttackDelay(0.1);

	if (m_flNextPrimaryAttack < UTIL_WeaponTimeBase())
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.1;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);
}

void CZorcher::SecondaryAttack(void){

}

BOOL CZorcher::Deploy()
{
	return DefaultDeploy("models/v_egon.mdl", "models/p_egon.mdl", ZRCH_DRAW, "hive");
}

void CZorcher::WeaponIdle(void)
{
	if (m_flTimeWeaponIdle > gpGlobals->time)
		return;

	int iAnim;
	float flRand = RANDOM_FLOAT(0, 1);
	if (flRand <= 0.75)
	{
		iAnim = ZRCH_IDLE1;
		m_flTimeWeaponIdle = gpGlobals->time + 30.0 / 16 * (2);
	}
	else if (flRand <= 0.875)
	{
		iAnim = ZRCH_IDLE1;
		m_flTimeWeaponIdle = gpGlobals->time + 40.0 / 16.0;
	}
	else
	{
		iAnim = ZRCH_IDLE1;
		m_flTimeWeaponIdle = gpGlobals->time + 35.0 / 16.0;
	}
	SendWeaponAnim(iAnim);
}