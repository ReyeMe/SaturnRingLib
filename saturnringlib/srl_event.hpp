#pragma once

#include "srl_base.hpp"
#include "srl_memory.hpp"
#include <functional>
#include <vector>

namespace SRL::Types
{
    /** @brief This proxy is used to interface with member functions
     * @tparam Args Member function argument types
     */
    template<typename ...Args>
    class MemberProxy
    {
    private:

        /** @brief Function reference
         */
        std::function<void(Args...)> function;

    public:

        /** @brief Destroy the member proxy
         */
        ~MemberProxy() = default;

        /** @brief Construct a new lambda proxy
         * @details Used to construct a proxy between instance lambda function and event
         * @code {.cpp}
         * // Define sample class
         * class TinyClass {
         *  uint32_t counter;
         * public:
         *  // Define lambda proxy
         *  SRL::Types::MemberProxy<> Proxy = SRL::Types::MemberProxy([this]() 
         *  {
         *      this->counter++;
         *  });
         * 
         *  // Attach to v-blank in constructor
         *  TinyClass() {
         *      SRL::Core::OnVblank += &this->Proxy;
         *  }
         * 
         *  // De-attach from v-blank in destructor
         *  ~TinyClass() {
         *      SRL::Core::OnVblank -= &this->Proxy;
         *  }
         * }
         * 
         * // Program start
         * int main() {
         *  SRL::Core::Initialize(HighColor::Colors::Black);
         * 
         *  // Create object
         *  TinyClass tiny();
         * 
         *  // Game loop
         *  while(1);
         * }
         * @endcode
         * @tparam Lambda Lambda type
         * @param lambda Lambda definition
         */
        template<typename Lambda>
        MemberProxy(Lambda&& lambda)
        {
            function = std::forward<Lambda>(lambda);
        }

        /** @brief Construct a new member proxy
         * @details Used to construct a proxy between instance member function (class bound function) and event
         * @code {.cpp}
         * // Define sample class
         * class TinyClass {
         *  uint32_t counter;
         * public:
         *  // Define proxy
         *  SRL::Types::MemberProxy<> Proxy = SRL::Types::MemberProxy(this, &TinyClass::Test);
         * 
         *  // Attach to v-blank in constructor
         *  TinyClass() {
         *      SRL::Core::OnVblank += &this->Proxy;
         *  }
         * 
         *  // De-attach from v-blank in destructor
         *  ~TinyClass() {
         *      SRL::Core::OnVblank -= &this->Proxy;
         *  }
         * 
         *  // Do our stuff inside this function
         *  void Test() {  
         *      this->counter++;
         *  }
         * }
         * 
         * // Program start
         * int main() {
         *  SRL::Core::Initialize(HighColor::Colors::Black);
         * 
         *  // Create object
         *  TinyClass tiny();
         * 
         *  // Game loop
         *  while(1);
         * }
         * @endcode
         * @tparam ClassName Member class
         * @param instance Member class instance
         * @param memberFunction Member function signature
         */
        template<typename ClassName>
        MemberProxy(ClassName* instance, void (ClassName::* memberFunction)(Args...))
        {
            function = [instance, memberFunction](Args... args)
            {
                (instance->*memberFunction)(args...);  // Call the member function on the instance
            };
        }

        /** @brief Invoke member function
         * @param args Function arguments
         */
        void Invoke(Args... args)
        {
            // std::function needs this stupid check or else the compiler explodes
            if (function)
            {
                function(args...);
            }
        }
    };

    /** @brief Event delegate
     * @tparam ArgumentTypes Event arguments
     */
    template<typename ...Args>
    class Event
    {
    public:

        /** @brief Static function signature
         */
        using CallbackStatic = void(*)(Args...);

        /** @brief Member function signature
         */
        using CallbackMember = MemberProxy<Args...>*;

    private:
        /** @brief Collection of static function callbacks
         */
        std::vector<CallbackStatic> staticCallbacks;

        /** @brief Collection of member function callbacks
         */
        std::vector<CallbackMember> memberCallbacks;

    public:

        /** @brief Construct a new event
         */
        Event()
        {
            this->staticCallbacks.reserve(1);
            this->memberCallbacks.reserve(1);
        }

        /** @brief Add static function callback
         * @param callback Static function callback
         * @return Event<Args...>& event object
         */
        Event<Args...>& operator+=(CallbackStatic callback)
        {
            if (this->staticCallbacks.size() == this->staticCallbacks.capacity())
            {
                this->staticCallbacks.reserve(this->staticCallbacks.size() + 1);
            }

            this->staticCallbacks.push_back(callback);
            return *this;
        }

        /** @brief Add member function callback
         * @param callback Member function proxy
         * @return Event<Args...>& event object
         */
        Event<Args...>& operator+=(CallbackMember callback)
        {
            if (this->memberCallbacks.size() == this->memberCallbacks.capacity())
            {
                this->memberCallbacks.reserve(this->memberCallbacks.size() + 1);
            }

            this->memberCallbacks.push_back(callback);
            return *this;
        }

        /** @brief Remove static callback
         * @param callback Static callback
         * @return Event<Args...>& Event class
         */
        Event<Args...>& operator-=(CallbackStatic callback)
        {
            auto it = std::ranges::find(this->staticCallbacks.begin(), this->staticCallbacks.end(), callback);

            if (it != this->staticCallbacks.end())
            {
                this->staticCallbacks.erase(it);
            }

            return *this;
        }

        /** @brief Remove member callback
         * @param callback Member callback proxy
         * @return Event<Args...>& Event class
         */
        Event<Args...>& operator-=(CallbackMember callback)
        {
            auto it = std::ranges::find(this->memberCallbacks.begin(), this->memberCallbacks.end(), callback);

            if (it != this->memberCallbacks.end())
            {
                this->memberCallbacks.erase(it);
            }

            return *this;
        }

        /** @brief Invoke all callbacks
         * @param args Invocation parameters
         */
        void Invoke(Args... args)
        {
            Event::InvokeList(this->staticCallbacks, [&](CallbackStatic callback) { callback(args...); });
            Event::InvokeList(this->memberCallbacks, [&](CallbackMember callback) { callback->Invoke(args...); });
        }

    private:
        /** @brief Calls the callbacks of a list, which they may change while they run (by index, not by iterator)
         * @details Callbacks added during the call run from the next Invoke; a callback removed before its turn does not run.
         * @param list Callbacks to call
         * @param call Calls one callback
         */
        template <typename List, typename Call>
        static void InvokeList(List& list, Call call)
        {
            size_t count = list.size();

            for (size_t index = 0; index < count && index < list.size(); index++)
            {
                const auto callback = list[index];

                if (callback)
                {
                    call(callback);
                }

                // The callback removed itself or an earlier one: the next callback moved into this slot
                if (index < list.size() && list[index] != callback)
                {
                    index--;
                    count--;
                }
            }
        }
    };
}