#include "StateMachine.h"
#include "State.h"
#include <assert.h>

using namespace std;
using namespace FSM;

StateMachine::StateMachine():
    m_sharedData(NULL)
{
    m_allStates.clear();
}

StateMachine::~StateMachine()
{
    //dtor
}

int StateMachine::add_state(State* state, bool recursive /*=true*/)
{
    string stateId = state->get_id();
    // check if the state is already added
    if (state_exists(stateId))
    {
        printf("%s\n", stateId.c_str());
        assert(!state_exists(stateId));
        return -1;
    }

    m_allStates[stateId] = state;

    int res;
    if (recursive)
    {
        for (auto childState : state->m_childStates)
        {
            res = add_state(childState);
            if (res == -1)
                return res;
        }
    }

    return 0;
}

int StateMachine::set_init_state(State* state)
{
    if (!state_exists(state->get_id()))
        return -1;

    if (!m_runningStates.empty())
    {
        assert(m_runningStates.empty());
        return -1;
    }

    return go_to_state(state->get_id());
}

int StateMachine::go_to_state(string stateId)
{
    int res = 0;

    if (!state_exists(stateId))
    {
        printf("Go to state not existed: %s\n", stateId.c_str());
        assert(0);
        return -1;
    }
    State* targetState = m_allStates[stateId];
    State* curState = NULL;

    while (!m_runningStates.empty())
    {
        curState = m_runningStates.top();

        if (curState->is_parent_of(targetState))
            break;

        res = curState->on_exit();
        if (res != 0)
            return res;

        m_runningStates.pop();
    }

    while (targetState != NULL)
    {
        m_runningStates.push(targetState);

        // target state's next state may change in on_enter
        res = targetState->on_enter();
        if (res != 0)
            return res;
        targetState = targetState->m_childStateCandidate;
    }

    return res;
}

void StateMachine::delete_all_states()
{
    for (auto statePair : m_allStates)
    {
        delete statePair.second;
    }

    m_allStates.clear();
}

State* StateMachine::get_active_state() const
{
    if (m_runningStates.empty())
        return NULL;

    return m_runningStates.top();
}

bool StateMachine::state_exists(string stateId)
{
    map<string, State*>::iterator itr = m_allStates.find(stateId);
    if (itr == m_allStates.end())
        return false;

    return true;
}
