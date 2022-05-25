#include <logging.hxx>

export import MainApplication.Base;

export module MainApplication.Default;


namespace Default {
	export struct Context: public Application::BaseContext {
		virtual bool KeepContextRunning() override {LOG_trace(__FUNCSIG__);static int val = 3; return val--;}

		const int IntField = 11;
	};

	export class AppInstance: public Application::BaseInstance<Default::Context> {
	public:
		virtual void setup (const std::span<char*> &argument_list) override {LOG_trace("{} - {} - {}", __FUNCSIG__, m_DoubleField, m_Context.IntField);}
		virtual void initializeVk () override {LOG_trace(__FUNCSIG__);}
		virtual void update (double update_latency) override {LOG_trace(__FUNCSIG__);}
		virtual void render (double render_latency) override {LOG_trace(__FUNCSIG__);}
		virtual void terminateVk () override {LOG_trace(__FUNCSIG__);}
		virtual void cleanup () override {LOG_trace(__FUNCSIG__);}

		const double m_DoubleField = 10.0;
	};
}