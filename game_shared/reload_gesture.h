#pragma once

// Shared by weapon simulation and the local ammunition inspection display.
class ReloadGesture
{
public:
	enum Action { None, Emergency, Tactical, Inspect };
	static constexpr double DoubleTapSeconds = 0.25;
	static constexpr double InspectSeconds = 0.6;

	void Reset(bool held = false)
	{
		m_down = held;
		m_consumed = held;
		m_secondTap = false;
		m_pressedAt = m_tappedAt = -1;
	}

	bool Pending() const { return m_down || m_tappedAt >= 0; }
	bool Inspecting() const { return m_down && m_consumed && m_pressedAt >= 0; }

	Action Update(bool held, double now)
	{
		if((m_pressedAt > now) || (m_tappedAt > now)) Reset(held);
		if(held && !m_down)
		{
			m_secondTap = m_tappedAt >= 0 && now - m_tappedAt <= DoubleTapSeconds;
			m_pressedAt = now;
			m_tappedAt = -1;
			m_consumed = false;
		}
		m_down = held;
		if(held && !m_consumed && now - m_pressedAt >= InspectSeconds)
		{
			m_consumed = true;
			m_secondTap = false;
			return Inspect;
		}
		if(!held && m_pressedAt >= 0)
		{
			const bool consumed = m_consumed;
			const bool longPress = now - m_pressedAt >= InspectSeconds;
			const bool second = m_secondTap;
			m_pressedAt = -1;
			m_secondTap = m_consumed = false;
			if(!consumed)
			{
				if(longPress) return Inspect;
				if(second) return Tactical;
				m_tappedAt = now;
			}
		}
		if(!held && m_tappedAt >= 0 && now - m_tappedAt >= DoubleTapSeconds)
		{
			m_tappedAt = -1;
			return Emergency;
		}
		return None;
	}

private:
	bool m_down = false, m_consumed = false, m_secondTap = false;
	double m_pressedAt = -1, m_tappedAt = -1;
};
