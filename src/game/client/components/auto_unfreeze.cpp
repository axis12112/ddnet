#include "auto_unfreeze.h"
#include <engine/shared/config.h>
#include <game/client/gameclient.h>
#include <game/collision.h>
#include <base/math.h>
#include <base/vmath.h>

#define MAX_BOUNCES 4
#define LASER_LEN 800.0f

// Исправляем ошибку компиляции (обязательная функция)
int CAutoUnfreeze::Sizeof() const 
{ 
    return sizeof(*this); 
}

void CAutoUnfreeze::OnReset()
{
    m_LastPos = vec2(0, 0);
}

bool CAutoUnfreeze::IsNearFreeze(vec2 Pos)
{
    // Проверка на заморозку в радиусе игрока
    for(int x = -1; x <= 1; x++)
    {
        for(int y = -1; y <= 1; y++)
        {
            if(m_pClient->Collision()->GetCollisionAt(Pos.x + x * 32, Pos.y + y * 32) & CCollision::COLFLAG_FREEZE)
                return true;
        }
    }
    return false;
}

vec2 CAutoUnfreeze::GetNormal(vec2 From, vec2 To)
{
    vec2 Dir = normalize(To - From);
    vec2 TestX = vec2(Dir.y, -Dir.x);
    vec2 TestY = vec2(-Dir.y, Dir.x);

    if(m_pClient->Collision()->GetCollisionAt(To.x + TestX.x * 4, To.y + TestX.y * 4) & CCollision::COLFLAG_SOLID)
        return TestX;
    if(m_pClient->Collision()->GetCollisionAt(To.x + TestY.x * 4, To.y + TestY.y * 4) & CCollision::COLFLAG_SOLID)
        return TestY;

    return vec2(0, -1);
}

bool CAutoUnfreeze::SimulateLaser(vec2 Pos, vec2 Dir, vec2 &OutHitPos, vec2 &OutDir)
{
    vec2 CurrentPos = Pos;
    vec2 CurrentDir = normalize(Dir);

    for(int i = 0; i < MAX_BOUNCES; i++)
    {
        vec2 To = CurrentPos + CurrentDir * LASER_LEN;
        vec2 Hit;
        if(m_pClient->Collision()->IntersectLine(CurrentPos, To, &Hit, nullptr))
        {
            vec2 Normal = GetNormal(CurrentPos, Hit);
            CurrentDir = CurrentDir - 2.0f * dot(CurrentDir, Normal) * Normal;
            CurrentPos = Hit;
        }
        else 
        { 
            break; 
        }
    }
    OutHitPos = CurrentPos;
    OutDir = CurrentDir;
    return true;
}

void CAutoUnfreeze::OnRender()
{
    if(!m_pClient->m_Snap.m_pLocalCharacter) 
        return;

    vec2 Pos = m_pClient->m_LocalCharacterPos;
    vec2 Vel = Pos - m_LastPos;
    m_LastPos = Pos;

    if(!IsNearFreeze(Pos)) 
        return;

    // Предсказываем, где мы будем через 4 тика
    vec2 Predicted = Pos + Vel * 4.0f;
    float BestScore = 1000.0f;
    vec2 BestDir = vec2(0, 0);

    for(float a = 0; a < pi * 2; a += 0.15f)
    {
        vec2 Dir = vec2(cosf(a), sinf(a));
        vec2 HitPos, OutDir;
        SimulateLaser(Pos, Dir, HitPos, OutDir);

        vec2 EndPos = HitPos + OutDir * 150.0f; 
        float Dist = distance(EndPos, Predicted);

        if(Dist < BestScore)
        {
            BestScore = Dist;
            BestDir = Dir;
        }
    }

    if(BestScore < 80.0f)
    {
        int Dummy = g_Config.m_ClDummy;
        m_pClient->m_Controls.m_aInputData[Dummy].m_TargetX = (int)(BestDir.x * 500.0f);
        m_pClient->m_Controls.m_aInputData[Dummy].m_TargetY = (int)(BestDir.y * 500.0f);
        m_pClient->m_Controls.m_aInputData[Dummy].m_WantedWeapon = 5 + 1; // Лазер
    }
}