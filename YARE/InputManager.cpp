#include "InputManager.h"

void InputManager::KeyDown(unsigned int keyIndex)
{
	m_keys[keyIndex] = true;
}

void InputManager::KeyUp(unsigned int keyIndex)
{
	m_keys[keyIndex] = false;
}

bool InputManager::IsKeyDown(unsigned int keyIndex) const
{
	return m_keys[keyIndex];
}

bool InputManager::IsLetterKeyDown() const
{
	for (unsigned int i = 65; i <= 90; ++i)
	{
		if (IsKeyDown(i))
			return true;
	}
	return false;
}

bool InputManager::IsNumberKeyDown() const
{
	for (unsigned int i = 48; i <= 57; ++i)
	{
		if (IsKeyDown(i))
			return true;
	}
	return false;
}

bool InputManager::IsAlphanumericKeyDown() const
{
	return IsLetterKeyDown() || IsNumberKeyDown();
}
