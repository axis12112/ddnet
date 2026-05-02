#ifndef GAME_CLIENT_COMPONENTS_AUTO_UNFREEZE_H
#define GAME_CLIENT_COMPONENTS_AUTO_UNFREEZE_H

#include <game/client/component.h>

class CAutoUnfreeze : public CComponent
{
    vec2 m_LastPos; // Храним прошлую позицию для расчета скорости

public:
    // Обязательная функция для DDNet (исправляет твою ошибку)
    virtual int Sizeof() const override { return sizeof(*this); }
    
    virtual void OnReset() override;
    virtual void OnRender() override; // В DDNet логика обычно пишется здесь

private:
    bool IsNearFreeze(vec2 Pos);
    vec2 GetNormal(vec2 From, vec2 To);
    bool SimulateLaser(vec2 Pos, vec2 Dir, vec2 &OutHitPos, vec2 &OutDir);
};

#endif