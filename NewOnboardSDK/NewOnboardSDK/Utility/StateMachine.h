#ifndef STATEMACHINE_H
#define STATEMACHINE_H

#include <stack>
#include <map>
#include <string>

namespace FSM{
class State;

class SharedData;

class StateMachine
{
    public:
        StateMachine();
        virtual ~StateMachine();

        int add_state(State*, bool = true);
        int set_init_state(State*);

        int go_to_state(std::string);
        // call in the end of the program
        void delete_all_states();

        State* get_active_state() const;

        void set_shared_data(SharedData* data) { m_sharedData = data; }
        SharedData* get_shared_data() const { return m_sharedData; }

    private:
        bool state_exists(std::string);
    private:
        std::stack<State*> m_runningStates;
        std::map<std::string, State*> m_allStates;
        SharedData* m_sharedData;
};

class SharedData
{
public:
    SharedData(){}
    virtual ~SharedData(){}
};




}

#endif // STATEMACHINE_H
