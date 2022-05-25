#include <logging.hxx>;

export import Core.KeyCodes;
export import Core.MouseCodes;
export module Core.Events;

#define EVENT_CLASS_TYPE(type) static consteval EventType GetStaticType() { return EventType::##type; }\
							  virtual EventType GetEventType() const override { return GetStaticType (); }\
							  virtual const char* GetName() const override { return #type; }
#define EVENT_CLASS_CATEGORY(category) virtual int GetCategoryFlags() const override { return category; }
#define BIT(x) (1 << x)

namespace NutCracker {
	export 
	enum class EventType
	{
		None = 0,
		WindowClose, WindowResize, WindowFocus, WindowLostFocus, WindowMoved,
		AppTick, AppUpdate, AppRender,
		KeyPressed, KeyReleased, KeyTyped,
		MouseButtonPressed, MouseButtonReleased, MouseMoved, MouseScrolled,
	};
	export 
	enum EventCategory
	{
		None = 0,
		APPLICATION    = BIT(0),
		INPUT          = BIT(1),
		KEYBOARD       = BIT(2),
		MOUSE          = BIT(3),
		MOUSE_BUTTON   = BIT(4),
	};
	export 
	class Event
	{
	public:
		const uint8_t WindowNum;
		bool Handled = false;
		
		Event (const uint8_t win_num)
			: WindowNum (win_num) {}

		virtual EventType GetEventType() const = 0;
		virtual const char* GetName() const = 0;
		virtual int GetCategoryFlags() const = 0;
		virtual std::string ToString() const { return GetName(); }

		inline bool IsInCategory(EventCategory category)
		{
			return GetCategoryFlags() & category;
		}
	};
	export 
	class EventDispatcher
	{
	public:
		EventDispatcher(Event &event)
			: m_Event(event)
		{}
		
		// F will be deduced by the compiler,  F -> bool (*func) (T&)
		template<typename T, typename F>
		bool Dispatch(const F& func)
		{
			if (m_Event.GetEventType() == T::GetStaticType())
			{
				m_Event.Handled = func (static_cast<T&>(m_Event));
				return true;
			}
			return false;
		}
		// F will be deduced by the compiler, F -> bool (*func) (Event&)
		template<EventCategory Flag, typename F>
		bool CategoryDispatch(const F& func)
		{
			if (m_Event.IsInCategory(Flag))
			{
				m_Event.Handled = func(m_Event);
				return true;
			}
			return false;
		}
	private:
		Event &m_Event;
	};
};

import <fmt/format.h>;

export 
inline std::ostream& operator<<(std::ostream& os, const NutCracker::Event &e)
{
	return os << fmt::format ("from Wnd:{:d}, {:s}", e.WindowNum, e.ToString());
}
export template <>
struct fmt::v8::detail::fallback_formatter<NutCracker::Event>
{
    constexpr auto parse(format_parse_context& ctx)
    {
        return ctx.end();
    }

    template <typename FormatContext>
    auto format(const NutCracker::Event& e, FormatContext& ctx)
    {
        return fmt::format_to(ctx.out(), "from Wnd:{:d}, {:s}", e.WindowNum, e.ToString());
    }
};

namespace NutCracker {
	export
	class WindowResizeEvent : public Event
	{
	public:
		WindowResizeEvent (const uint8_t win_num, uint32_t width, uint32_t height)
			: Event (win_num), m_Width(width), m_Height(height) {}

		inline uint32_t GetWidth() const { return m_Width; }
		inline uint32_t GetHeight() const { return m_Height; }

		std::string ToString() const override
		{
			return fmt::format("WindowResizeEvent: {:d}, {:d}", m_Width, m_Height);
		}

		EVENT_CLASS_TYPE(WindowResize)
		EVENT_CLASS_CATEGORY(EventCategory::APPLICATION)
	private:
		uint32_t m_Width, m_Height;
	};
	export
	class WindowCloseEvent : public Event
	{
	public:
		WindowCloseEvent (const uint8_t win_num)
			: Event (win_num) {}

		EVENT_CLASS_TYPE(WindowClose)
		EVENT_CLASS_CATEGORY(EventCategory::APPLICATION)
	};
	export
	class AppTickEvent : public Event
	{
	public:
		AppTickEvent (const uint8_t win_num)
			: Event (win_num) {}

		EVENT_CLASS_TYPE(AppTick)
		EVENT_CLASS_CATEGORY(EventCategory::APPLICATION)
	};
	export
	class AppUpdateEvent : public Event
	{
	public:
		AppUpdateEvent (const uint8_t win_num)
			: Event (win_num) {}

		EVENT_CLASS_TYPE(AppUpdate)
		EVENT_CLASS_CATEGORY(EventCategory::APPLICATION)
	};
	export
	class AppRenderEvent : public Event
	{
	public:
		AppRenderEvent (const uint8_t win_num)
			: Event (win_num) {}

		EVENT_CLASS_TYPE(AppRender)
		EVENT_CLASS_CATEGORY(EventCategory::APPLICATION)
	};
};

namespace NutCracker {
	export
	class MouseMovedEvent : public Event
	{
	public:
		MouseMovedEvent (const uint8_t win_num, const float x, const float y)
			: Event(win_num), m_MouseX(x), m_MouseY(y) {}

		inline float GetX() const { return m_MouseX; }
		inline float GetY() const { return m_MouseY; }

		std::string ToString() const override
		{
			return fmt::format("MouseMovedEvent: {:f}, {:f}", m_MouseX, m_MouseY);
		}

		EVENT_CLASS_TYPE(MouseMoved)
		EVENT_CLASS_CATEGORY(EventCategory::MOUSE | EventCategory::INPUT)
	private:
		float m_MouseX, m_MouseY;
	};

	export
	class MouseScrolledEvent : public Event
	{
	public:
		MouseScrolledEvent(const uint8_t win_num, const float xOffset, const float yOffset)
			: Event(win_num), m_XOffset(xOffset), m_YOffset(yOffset) {}

		inline float GetXOffset() const { return m_XOffset; }
		inline float GetYOffset() const { return m_YOffset; }

		std::string ToString() const override
		{
			return fmt::format ("MouseScrolledEvent: {:f}, {:f}", GetXOffset(), GetYOffset());
		}

		EVENT_CLASS_TYPE(MouseScrolled)
		EVENT_CLASS_CATEGORY(EventCategory::MOUSE | EventCategory::INPUT)
	private:
		float m_XOffset, m_YOffset;
	};

	export
	class MouseButtonEvent : public Event
	{
	public:
		inline MouseCode GetMouseButton () const { return m_Button; }

		EVENT_CLASS_CATEGORY(EventCategory::MOUSE | EventCategory::INPUT | EventCategory::MOUSE_BUTTON)
	protected:
		MouseButtonEvent(const uint8_t win_num, const int button)
			: Event(win_num), m_Button(button) {}

		MouseCode m_Button;
	};

	export
	class MouseButtonPressedEvent : public MouseButtonEvent
	{
	public:
		MouseButtonPressedEvent(const uint8_t win_num, const MouseCode button)
			: MouseButtonEvent(win_num, button) {}

		std::string ToString() const override
		{
			return fmt::format ("MouseButtonPressedEvent: {:d}", m_Button);
		}

		EVENT_CLASS_TYPE(MouseButtonPressed)
	};

	export
	class MouseButtonReleasedEvent : public MouseButtonEvent
	{
	public:
		MouseButtonReleasedEvent(const uint8_t win_num, const MouseCode button)
			: MouseButtonEvent(win_num, button) {}

		std::string ToString() const override
		{
			return fmt::format ("MouseButtonReleasedEvent: {:d}", m_Button);
		}

		EVENT_CLASS_TYPE(MouseButtonReleased)
	};
};

namespace NutCracker {
	export
	class KeyEvent : public Event
	{
	public:
		inline KeyCode GetKeyCode() const { return m_KeyCode; }

		EVENT_CLASS_CATEGORY (EventCategory::KEYBOARD | EventCategory::INPUT)
	protected:
		KeyEvent (const uint8_t win_num, const KeyCode keycode)
			: Event (win_num), m_KeyCode(keycode) {}

		KeyCode m_KeyCode;
	};

	export
	class KeyPressedEvent : public KeyEvent
	{
	public:
		KeyPressedEvent (const uint8_t win_num, const KeyCode keycode, const int repeatCount)
			: KeyEvent (win_num, keycode), m_RepeatCount(repeatCount) {}

		inline int GetRepeatCount() const { return m_RepeatCount; }

		std::string ToString() const override
		{
			return fmt::format ("KeyPressedEvent: {:d}[\'{}\'] {} repeats",  m_KeyCode, char (m_KeyCode), m_RepeatCount);
		}

		EVENT_CLASS_TYPE(KeyPressed)
	private:
		int m_RepeatCount;
	};

	export
	class KeyReleasedEvent : public KeyEvent
	{
	public:
		KeyReleasedEvent (const uint8_t win_num, const KeyCode keycode)
			: KeyEvent (win_num, keycode) {}

		std::string ToString() const override
		{
			return fmt::format ("KeyReleasedEvent: {:d}[\'{}\']", m_KeyCode, char (m_KeyCode));
		}

		EVENT_CLASS_TYPE(KeyReleased)
	};

	export
	class KeyTypedEvent : public KeyEvent
	{
	public:
		KeyTypedEvent (const uint8_t win_num, const KeyCode keycode)
			: KeyEvent (win_num, keycode) {}

		std::string ToString() const override
		{
			return fmt::format ("KeyTypedEvent: {:d}[\'{}\']", m_KeyCode, char (m_KeyCode));
		}

		EVENT_CLASS_TYPE(KeyTyped)
	};
};