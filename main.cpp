#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/msvc_sink.h>
#include <vector>
#include <memory>
#include <iostream>

void static setupSpdlog() {
    try {
        // Two sinks for output to file and visual studio output console
        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("lexicon.log", true);
        auto msvc_sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();

        file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%f] [%l] [%t] %v");
        msvc_sink->set_pattern("[%H:%M:%S.%f] [%l] %v");

        std::vector<spdlog::sink_ptr> sinks;
        sinks.push_back(file_sink);
        sinks.push_back(msvc_sink);

        auto logger = std::make_shared<spdlog::logger>("multi_sink_logger", sinks.begin(), sinks.end());
        spdlog::set_default_logger(logger);
        spdlog::set_level(spdlog::level::debug);
        spdlog::flush_on(spdlog::level::info);

        spdlog::info("Spdlog initialized successfully.");
    } catch (const spdlog::spdlog_ex& ex) {
        std::cout << "Spdlog initialization failed: " << ex.what() << std::endl;
    }
}

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    //qmlRegisterType<ModModel>("Lexicon.Network", 1, 0, "ModModel");

    QQmlApplicationEngine engine;
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("ModdingLexicon", "Main");

    setupSpdlog();
    spdlog::info("Application started.");

    return app.exec();
}