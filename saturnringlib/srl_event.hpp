#pragma once

#include "srl_base.hpp"
#include "srl_memory.hpp"
#include <vector>

namespace SRL::Types
{
	/** @brief Event delegate
	 * @tparam ArgumentTypes Event arguments
	 */
	template<typename ...ArgumentTypes>
	class Event
	{
	private:

		/** @brief Event callbacks
		 */
		std::vector<void (*)(ArgumentTypes...)> callbacks;

	public:

		/** @brief Construct a new Event object
		 */
		Event() {}

		/** @brief Destroy the Event object
		 */
		~Event() {}

		/** @brief Invoke event
		 * @param args Event arguments
		 */
		void Invoke(ArgumentTypes... args)
		{
			for (auto callback : this->callbacks)
			{
				if (callback != nullptr)
				{
					callback(args...);
				}
			}
		}

		/** @brief Attach callback
		 * @param callback Event callback
		 * @return Event<ArgumentTypes...>& Event object
		 */
		Event<ArgumentTypes...>& operator+=(void (*callback)(ArgumentTypes...))
		{
			auto it = std::ranges::find(this->callbacks.begin(), this->callbacks.end(), callback);

			if (it == this->callbacks.end())
			{
				this->callbacks.push_back(static_cast<void (*)(ArgumentTypes...)>(callback));
			}

			return *this;
		}

		/** @brief Remove callback
		 * @param callback Event callback
		 * @return Event<ArgumentTypes...>& Event object
		 */
		Event<ArgumentTypes...>& operator-=(void (*callback)(ArgumentTypes...))
		{
			auto it = std::ranges::find(this->callbacks.begin(), this->callbacks.end(), callback);

			if (it != this->callbacks.end())
			{
				this->callbacks.erase(it);
			}

			return *this;
		}
	};
}