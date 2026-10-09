#ifndef _UNDO_REDO_MANAGER_H_
#define _UNDO_REDO_MANAGER_H_

#include <vector>
#include <cstddef>

template<typename T>
class UndoRedoManager
{
public:
	UndoRedoManager(size_t max_states = 100)
		: m_maxStates(max_states),
		  m_savedStateIndex(-1)
	{
	}

	void PushState(const T& state)
	{
		m_redoStack.clear();
		m_undoStack.push_back(state);
		if (m_undoStack.size() > m_maxStates)
		{
			m_undoStack.erase(m_undoStack.begin());
		}
		if (m_savedStateIndex >= static_cast<int>(m_undoStack.size()))
		{
			m_savedStateIndex = -1;
		}
	}

	bool Undo(T& current_state)
	{
		if (!CanUndo())
		{
			return false;
		}
		m_redoStack.push_back(current_state);
		current_state = m_undoStack.back();
		m_undoStack.pop_back();
		return true;
	}

	bool Redo(T& current_state)
	{
		if (!CanRedo())
		{
			return false;
		}
		m_undoStack.push_back(current_state);
		current_state = m_redoStack.back();
		m_redoStack.pop_back();
		return true;
	}

	void Clear()
	{
		m_undoStack.clear();
		m_redoStack.clear();
		m_savedStateIndex = -1;
	}

	bool CanUndo() const
	{
		return !m_undoStack.empty();
	}

	bool CanRedo() const
	{
		return !m_redoStack.empty();
	}

	void MarkSaved()
	{
		m_savedStateIndex = static_cast<int>(m_undoStack.size()) - 1;
	}

	bool IsModified() const
	{
		return static_cast<int>(m_undoStack.size()) - 1 != m_savedStateIndex;
	}

private:
	std::vector<T> m_undoStack;
	std::vector<T> m_redoStack;
	size_t m_maxStates;
	int m_savedStateIndex;
};

#endif // _UNDO_REDO_MANAGER_H_
