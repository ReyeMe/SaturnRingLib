#pragma once

#include "srl_base.hpp"
#include "srl_memory.hpp"
#include <vector>

namespace SRL::Types
{
    /** @brief Proxy used to interface with static functions, it is not needed to be used by the user
     * @tparam ArgumentTypes Function arguments
     */
    template<typename ...ArgumentTypes>
    class AbstractProxy
    {
    public:
        /** @brief Invoke bound function
         * @param args Function arguments
         */
        virtual void Invoke(ArgumentTypes... args) = 0;
    };

    /** @brief Proxy used to interface with class member functions
     * @tparam C Member function class
     * @tparam ArgumentTypes Member function arguments
     */
    template<class C, typename ...ArgumentTypes>
    class MemberProxy : public AbstractProxy<ArgumentTypes...>
    {
    public:
        /** @brief Parent object
         */
        C* Object;

        /** @brief Function reference
         */
        void (C::*Function)(ArgumentTypes...);

        /** @brief Construct a new bound function proxy
         * @param object Bound function parent object
         * @param function Bound function
         */
        MemberProxy(C* object, void (C::*function)(ArgumentTypes...)) : Object(object), Function(function) { }

        /** @brief Destroy the bound function proxy
         */
        ~MemberProxy() { }

        /** @brief Invoke bound function
         * @param args Function arguments
         */
        void Invoke(ArgumentTypes... args) override
        {
            ((*this->Object).*this->Function)(args...);
        }
    };

    /** @brief Event delegate
     * @tparam ArgumentTypes Event arguments
     */
    template<typename ...Args>
    class Event
    {
    public:
        using CallbackStatic = void(*)(Args...);
        using CallbackMember = AbstractProxy<Args...>*;

    private:
        std::vector<CallbackStatic> staticCallbacks;
        std::vector<CallbackMember> memberCallbacks;

    public:

        // Add static callback
        Event<Args...>& operator+=(CallbackStatic callback)
        {
            auto it = std::ranges::find(this->staticCallbacks.begin(), this->staticCallbacks.end(), callback);
 
			if (it == this->staticCallbacks.end())
			{
				this->staticCallbacks.push_back(static_cast<CallbackStatic>(callback));
			}
 
			return *this;
        }

        // Add member callback
        Event<Args...>& operator+=(CallbackMember callback) {
            if (callback) {
                this->memberCallbacks.push_back(callback);
            }

            return *this;
        }

        // Remove static callback
        Event<Args...>& operator-=(CallbackStatic callback)
        {
			auto it = std::ranges::find(this->staticCallbacks.begin(), this->staticCallbacks.end(), callback);
 
			if (it != this->staticCallbacks.end())
			{
				this->staticCallbacks.erase(it);
			}

            return *this;
        }

        // Remove member callback
        Event<Args...>& operator-=(CallbackMember callback)
        {
			auto it = std::ranges::find(this->memberCallbacks.begin(), this->memberCallbacks.end(), callback);
 
			if (it != this->memberCallbacks.end())
			{
				this->memberCallbacks.erase(it);
			}

            return *this;
        }

        // Invoke all callbacks
        void Invoke(Args... args)
        {
            for (const auto& callback : this->staticCallbacks)
            {
                if (callback)
                {
                    callback(args...);
                }
            }

            for (const auto& callback : this->memberCallbacks)
            {
                if (callback)
                {
                    callback->Invoke(args...);
                }
            }
        }
    };
}