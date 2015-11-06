#ifndef STATE_H
#define STATE_H

#include <vector>
#include <string>

namespace FSM{

class StateMachine;

class State
{
    friend class StateMachine;
    public:
        State(std::string, StateMachine*);
        virtual ~State();

        std::string get_id() const { return m_id; }
        StateMachine* get_fsm() const { return m_fsm; }

        void add_child(State*);
        bool is_parent_of(State*);
        void set_candidate(State*);

    protected:
        virtual int on_enter() = 0;
        virtual int on_exit() = 0;

    private:
        std::string m_id;
        StateMachine* m_fsm;
        std::vector<State*> m_childStates;
        State* m_childStateCandidate;
};

}
#endif // STATE_H
