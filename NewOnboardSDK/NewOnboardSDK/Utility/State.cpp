#include "State.h"
#include "StateMachine.h"
#include <algorithm>
#include <cassert>

using namespace std;
using namespace FSM;

State::State(string id, StateMachine* fsm):
    m_id(id), m_fsm(fsm), m_childStateCandidate(NULL)
{
    m_fsm->add_state(this, false);
}

State::~State()
{
    m_childStates.clear();
}

void State::add_child(State* childState)
{
    m_childStates.push_back(childState);
}

bool State::is_parent_of(State* state)
{
    vector<State*>::iterator itr = find(m_childStates.begin(), m_childStates.end(), state);
    if (itr == m_childStates.end())
        return false;
    return true;
}

void State::set_candidate(State* state)
{
    assert(is_parent_of(state));
    m_childStateCandidate = state;
}
