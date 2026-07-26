#include "hud.h"
#include "parsemsg.h"
#include "utils.h"

DECLARE_MESSAGE(m_Stamina, Stamina);

int CHudStamina::Init()
{
	HOOK_MESSAGE(Stamina);
	gHUD.AddHudElem(this);
	m_iFlags = HUD_ACTIVE;
	Reset();
	return 1;
}

void CHudStamina::Reset()
{
	m_iStamina = 100;
	m_bDraining = false;
}

int CHudStamina::MsgFunc_Stamina(const char* name, int size, void* data)
{
	BEGIN_READ(name, data, size);
	const int rawStamina = READ_BYTE();
	m_iStamina = Q_max(0, Q_min(100, rawStamina));
	m_bDraining = READ_BYTE() != 0;
	return 1;
}

int CHudStamina::Draw(float time)
{
	if (m_iStamina >= 100 && !m_bDraining)
		return 1;

	const float fraction = m_iStamina / 100.0f;
	const int width = Q_max(160, (int)(ScreenWidth * 0.24f));
	const int height = 10;
	const int x = (ScreenWidth - width) / 2;
	const int y = ScreenHeight - Q_max(42, (int)(ScreenHeight * 0.07f));
	const int red = (int)(255.0f * (1.0f - fraction));
	const int green = (int)(220.0f * fraction);

	FillRGBA(x - 2, y - 2, width + 4, height + 4, 0, 0, 0, 190);
	FillRGBA(x, y, width, height, 35, 35, 35, 190);
	if (m_iStamina > 0)
		FillRGBA(x, y, Q_max(1, (int)(width * fraction)), height, red, green, 0, 235);
	return 1;
}
