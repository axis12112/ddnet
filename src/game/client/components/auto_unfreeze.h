#ifndef GAME_CLIENT_COMPONENTS_AUTO_UNFREEZE_H
#define GAME_CLIENT_COMPONENTS_AUTO_UNFREEZE_H

#include <game/client/component.h>

class CAutoUnfreeze : public CComponent
{
    vec2 m_LastPos; // Храним прошлую позицию для расчета скорости

public:
    virtual void OnUpdate() override; // В Teeworlds используется OnUpdate вместо OnTick
    virtual void OnReset() override;

private:
    bool IsNearFreeze(vec2 Pos);
    vec2 GetNormal(vec2 From, vec2 To);
    bool SimulateLaser(vec2 Pos, vec2 Dir, vec2 &OutHitPos, vec2 &OutDir);
};

#endif