#include "auto_unfreeze.h"
#include <engine/shared/config.h>
#include <game/client/gameclient.h>
#include <game/collision.h>
#include <base/math.h>
#include <base/vmath.h>

#define MAX_BOUNCES 5
#define LASER_LEN 800.0f
#define PLAYER_RADIUS 14.0f // Половина хитбокса тиворлда

void CAutoUnfreeze::OnReset()
{
    m_LastPos = vec2(0, 0);
    m_LastFireTime = 0;
}

// Проверка: мы действительно застряли на фризе?
bool CAutoUnfreeze::IsFrozen(vec2 Pos)
{
    // Проверяем 5 точек (центр и края), чтобы точно знать, что мы во фризе
    int Tiles[] = {
        m_pClient->Collision()->GetCollisionAt(Pos.x, Pos.y),
        m_pClient->Collision()->GetCollisionAt(Pos.x-14, Pos.y-14),
        m_pClient->Collision()->GetCollisionAt(Pos.x+14, Pos.y+14)
    };
    for(int t : Tiles)
        if(t & CCollision::COLFLAG_FREEZE) return true;
    return false;
}

// Точный расчет нормали поверхности
vec2 CAutoUnfreeze::GetNormal(vec2 HitPos)
{
    vec2 Directions[4] = { vec2(-1, 0), vec2(1, 0), vec2(0, -1), vec2(0, 1) };
    for(auto &Dir : Directions)
    {
        if(m_pClient->Collision()->GetCollisionAt(HitPos.x + Dir.x * 2, HitPos.y + Dir.y * 2) & CCollision::COLFLAG_SOLID)
            return -Dir;
    }
    return vec2(0, -1);
}

float CAutoUnfreeze::ClosestDistPointLine(vec2 Pos, vec2 LineStart, vec2 LineEnd)
{
    vec2 LineDir = LineEnd - LineStart;
    float l2 = length_squared(LineDir);
    if(l2 == 0.0f) return distance(Pos, LineStart);
    float t = clamp(dot(Pos - LineStart, LineDir) / l2, 0.0f, 1.0f);
    return distance(Pos, LineStart + t * LineDir);
}

void CAutoUnfreeze::OnRender()
{
    // Работаем только если мы в игре и за правильного персонажа
    if(!m_pClient->m_Snap.m_pLocalCharacter) return;

    vec2 Pos = m_pClient->m_LocalCharacterPos;
    vec2 Vel = Pos - m_LastPos;
    m_LastPos = Pos;

    // Активация: если мы на фризе и скорость упала (значит застряли)
    if(!IsFrozen(Pos) || length(Vel) > 0.5f) return;

    int Dummy = g_Config.m_ClDummy;
    vec2 BestDir = vec2(0, 0);
    float BestDist = 100.0f;
    bool Found = false;

    // Сканируем углы. Шаг 0.05f (~3 градуса) для высокой точности
    for(float a = 0; a < pi * 2; a += 0.05f)
    {
        vec2 CurrentPos = Pos;
        vec2 CurrentDir = vec2(cosf(a), sinf(a));
        
        for(int b = 0; b < MAX_BOUNCES; b++)
        {
            vec2 To = CurrentPos + CurrentDir * LASER_LEN;
            vec2 Hit;
            if(m_pClient->Collision()->IntersectLine(CurrentPos, To, &Hit, nullptr))
            {
                if(b > 0) // Начиная со второго сегмента (рикошета)
                {
                    float d = ClosestDistPointLine(Pos, CurrentPos, Hit);
                    if(d < PLAYER_RADIUS && d < BestDist)
                    {
                        BestDist = d;
                        BestDir = vec2(cosf(a), sinf(a));
                        Found = true;
                        if(d < 2.0f) break; // Почти идеальное попадание
                    }
                }
                vec2 Normal = GetNormal(Hit);
                CurrentDir = CurrentDir - 2.0f * dot(CurrentDir, Normal) * Normal;
                CurrentPos = Hit + CurrentDir * 0.5f;
            }
            else break;
        }
        if(Found && BestDist < 5.0f) break; 
    }

    if(Found)
    {
        // Наводимся
        m_pClient->m_Controls.m_aInputData[Dummy].m_TargetX = (int)(BestDir.x * 512.0f);
        m_pClient->m_Controls.m_aInputData[Dummy].m_TargetY = (int)(BestDir.y * 512.0f);
        m_pClient->m_Controls.m_aInputData[Dummy].m_WantedWeapon = 5 + 1; // Laser

        // Стреляем с задержкой, чтобы сервер успел принять поворот прицела
        int64_t Now = time_get();
        if(m_pClient->m_Snap.m_pLocalCharacter->m_Weapon == 5 && Now > m_LastFireTime + time_freq() / 10)
        {
            m_pClient->m_Controls.m_aInputData[Dummy].m_Fire++;
            m_LastFireTime = Now;
        }
    }
}