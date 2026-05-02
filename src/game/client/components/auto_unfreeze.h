#ifndef GAME_CLIENT_COMPONENTS_AUTO_UNFREEZE_H
#define GAME_CLIENT_COMPONENTS_AUTO_UNFREEZE_H

#include <game/client/component.h>

class CAutoUnfreeze : public CComponent
{
    vec2 m_LastPos;
    int64_t m_LastFireTime; // Чтобы не спамить выстрелами слишком часто

public:
    virtual int Sizeof() const override { return sizeof(*this); }
    virtual void OnReset() override;
    virtual void OnRender() override;

private:
    bool IsFrozen(vec2 Pos);
    vec2 GetNormal(vec2 HitPos);
    float ClosestDistPointLine(vec2 Pos, vec2 LineStart, vec2 LineEnd);
};

#endif