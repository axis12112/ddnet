#include "auto_unfreeze.h"
#include <base/math.h>
#include <base/system.h>
#include <base/vmath.h>
#include <engine/shared/config.h>
#include <game/client/gameclient.h>
#include <game/collision.h>
#include <game/mapitems.h>

#define MAX_BOUNCES 5
#define LASER_LEN 800.0f
#define PLAYER_RADIUS 14.0f

void CAutoUnfreeze::OnReset()
{
	m_LastPos = vec2(0, 0);
	m_LastFireTime = 0;
}

bool CAutoUnfreeze::IsFrozen(vec2 Pos)
{
	// Используем GetCollisionAt из твоего файла collision.h
	int Tile = GameClient()->Collision()->GetCollisionAt(Pos.x, Pos.y);
	return (Tile == TILE_FREEZE);
}

vec2 CAutoUnfreeze::GetNormal(vec2 HitPos)
{
	vec2 Directions[4] = {vec2(-1, 0), vec2(1, 0), vec2(0, -1), vec2(0, 1)};
	for(auto &Dir : Directions)
	{
		// Используем IsSolid из твоего collision.h
		if(GameClient()->Collision()->IsSolid(round_to_int(HitPos.x + Dir.x * 2), round_to_int(HitPos.y + Dir.y * 2)))
		{
			return -Dir;
		}
	}
	return vec2(0, -1);
}

float CAutoUnfreeze::ClosestDistPointLine(vec2 Pos, vec2 LineStart, vec2 LineEnd)
{
	vec2 LineDir = LineEnd - LineStart;
	float l2 = length_squared(LineDir);
	if(l2 == 0.0f)
		return length(Pos - LineStart);
	float t = dot(Pos - LineStart, LineDir) / l2;
	if(t < 0.0f)
		t = 0.0f;
	if(t > 1.0f)
		t = 1.0f;
	vec2 Projection = LineStart + LineDir * t;
	return length(Pos - Projection);
}

void CAutoUnfreeze::OnRender()
{
	if(!GameClient()->m_Snap.m_pLocalCharacter)
	{
		return;
	}

	vec2 Pos = GameClient()->m_LocalCharacterPos;
	vec2 Vel = Pos - m_LastPos;
	m_LastPos = Pos;

	// FLY-UNFREEZE: предсказываем позицию, чтобы успеть выстрелить ДО заморозки
	vec2 PredictedPos = Pos + Vel * 1.5f;

	if(!IsFrozen(PredictedPos) && !IsFrozen(Pos))
	{
		return;
	}

	int Dummy = g_Config.m_ClDummy;
	vec2 BestDir = vec2(0, 0);
	float BestDist = 100.0f;
	bool Found = false;

	for(float a = 0; a < 6.2831f; a += 0.05f)
	{
		vec2 CurrentPos = Pos;
		vec2 CurrentDir = vec2(cosf(a), sinf(a));

		for(int b = 0; b < MAX_BOUNCES; b++)
		{
			vec2 To = CurrentPos + CurrentDir * LASER_LEN;
			vec2 Hit;
			if(GameClient()->Collision()->IntersectLine(CurrentPos, To, &Hit, nullptr))
			{
				if(b > 0)
				{
					float d = ClosestDistPointLine(PredictedPos, CurrentPos, Hit);
					if(d < PLAYER_RADIUS && d < BestDist)
					{
						BestDist = d;
						BestDir = vec2(cosf(a), sinf(a));
						Found = true;
						if(d < 2.0f)
						{
							break;
						}
					}
				}
				vec2 Normal = GetNormal(Hit);
				CurrentDir = CurrentDir - Normal * 2.0f * dot(CurrentDir, Normal);
				CurrentPos = Hit + CurrentDir * 0.5f;
			}
			else
			{
				break;
			}
		}
		if(Found && BestDist < 5.0f)
		{
			break;
		}
	}

	if(Found)
	{
		GameClient()->m_Controls.m_aInputData[Dummy].m_TargetX = (int)(BestDir.x * 512.0f);
		GameClient()->m_Controls.m_aInputData[Dummy].m_TargetY = (int)(BestDir.y * 512.0f);
		GameClient()->m_Controls.m_aInputData[Dummy].m_WantedWeapon = 5 + 1;

		int64_t Now = time_get();
		if(GameClient()->m_Snap.m_pLocalCharacter->m_Weapon == 5 && Now > m_LastFireTime + time_freq() / 15)
		{
			GameClient()->m_Controls.m_aInputData[Dummy].m_Fire++;
			m_LastFireTime = Now;
		}
	}
}