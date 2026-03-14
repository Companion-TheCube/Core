if(NOT DEFINED WEBSOCKETPP_SOURCE_DIR)
    message(FATAL_ERROR "WEBSOCKETPP_SOURCE_DIR must be set")
endif()

function(replace_in_file relpath search replace)
    set(path "${WEBSOCKETPP_SOURCE_DIR}/${relpath}")
    file(READ "${path}" content)

    string(FIND "${content}" "${search}" search_pos)
    if(search_pos EQUAL -1)
        string(FIND "${content}" "${replace}" replace_pos)
        if(replace_pos EQUAL -1)
            message(FATAL_ERROR "Expected pattern not found in ${relpath}")
        endif()
        return()
    endif()

    string(REPLACE "${search}" "${replace}" updated "${content}")
    if(NOT "${updated}" STREQUAL "${content}")
        file(WRITE "${path}" "${updated}")
    endif()
endfunction()

replace_in_file(
    "websocketpp/transport/asio/connection.hpp"
    "typedef lib::asio::io_service * io_service_ptr;"
    "typedef lib::asio::io_context * io_service_ptr;"
)
replace_in_file(
    "websocketpp/transport/asio/connection.hpp"
    "typedef lib::shared_ptr<lib::asio::io_service::strand> strand_ptr;"
    "typedef lib::shared_ptr<lib::asio::io_context::strand> strand_ptr;"
)
replace_in_file(
    "websocketpp/transport/asio/connection.hpp"
    "new lib::asio::io_service::strand(*io_service)"
    "new lib::asio::io_context::strand(*io_service)"
)
replace_in_file(
    "websocketpp/transport/asio/connection.hpp"
    "m_strand->wrap("
    "lib::asio::bind_executor(*m_strand, "
)
replace_in_file(
    "websocketpp/transport/asio/connection.hpp"
    "m_io_service->post("
    "lib::asio::post(*m_io_service, "
)
replace_in_file(
    "websocketpp/transport/asio/connection.hpp"
    "->expires_from_now()"
    "->expiry() - lib::asio::steady_timer::clock_type::now()"
)

replace_in_file(
    "websocketpp/transport/asio/endpoint.hpp"
    "typedef lib::asio::io_service * io_service_ptr;"
    "typedef lib::asio::io_context * io_service_ptr;"
)
replace_in_file(
    "websocketpp/transport/asio/endpoint.hpp"
    "typedef lib::shared_ptr<lib::asio::io_service::work> work_ptr;"
    "typedef lib::shared_ptr<lib::asio::executor_work_guard<lib::asio::io_context::executor_type>> work_ptr;"
)
replace_in_file(
    "websocketpp/transport/asio/endpoint.hpp"
    "lib::unique_ptr<lib::asio::io_service> service(new lib::asio::io_service());"
    "lib::unique_ptr<lib::asio::io_context> service(new lib::asio::io_context());"
)
replace_in_file(
    "websocketpp/transport/asio/endpoint.hpp"
    "lib::auto_ptr<lib::asio::io_service> service(new lib::asio::io_service());"
    "lib::auto_ptr<lib::asio::io_context> service(new lib::asio::io_context());"
)
replace_in_file(
    "websocketpp/transport/asio/endpoint.hpp"
    "lib::asio::io_service & get_io_service() {"
    "lib::asio::io_context & get_io_service() {"
)
replace_in_file(
    "websocketpp/transport/asio/endpoint.hpp"
    "lib::asio::socket_base::max_connections"
    "lib::asio::socket_base::max_listen_connections"
)
replace_in_file(
    "websocketpp/transport/asio/endpoint.hpp"
    "m_work.reset(new lib::asio::io_service::work(*m_io_service));"
    "m_work.reset(new lib::asio::executor_work_guard<lib::asio::io_context::executor_type>(m_io_service->get_executor()));"
)
replace_in_file(
    "websocketpp/transport/asio/endpoint.hpp"
    [=[        tcp::resolver::query query(host, service);
        tcp::resolver::iterator endpoint_iterator = r.resolve(query);
        tcp::resolver::iterator end;
        if (endpoint_iterator == end) {
            m_elog->write(log::elevel::library,
                "asio::listen could not resolve the supplied host or service");
            ec = make_error_code(error::invalid_host_service);
            return;
        }
        listen(*endpoint_iterator,ec);]=]
    [=[        auto endpoints = r.resolve(host, service);
        auto endpoint_iterator = endpoints.begin();
        if (endpoint_iterator == endpoints.end()) {
            m_elog->write(log::elevel::library,
                "asio::listen could not resolve the supplied host or service");
            ec = make_error_code(error::invalid_host_service);
            return;
        }
        listen(endpoint_iterator->endpoint(),ec);]=]
)
replace_in_file(
    "websocketpp/transport/asio/endpoint.hpp"
    "        tcp::resolver::query query(host,port);\n\n"
    ""
)
replace_in_file(
    "websocketpp/transport/asio/endpoint.hpp"
    "            m_resolver->async_resolve(\n                query,\n"
    "            m_resolver->async_resolve(\n                host,\n                port,\n"
)
replace_in_file(
    "websocketpp/transport/asio/endpoint.hpp"
    "tcon->get_strand()->wrap("
    "lib::asio::bind_executor(*tcon->get_strand(), "
)
replace_in_file(
    "websocketpp/transport/asio/endpoint.hpp"
    "        connect_handler callback, lib::asio::error_code const & ec,\n        lib::asio::ip::tcp::resolver::iterator iterator)"
    "        connect_handler callback, lib::asio::error_code const & ec,\n        lib::asio::ip::tcp::resolver::results_type results)"
)
replace_in_file(
    "websocketpp/transport/asio/endpoint.hpp"
    [=[            lib::asio::ip::tcp::resolver::iterator it, end;
            for (it = iterator; it != end; ++it) {
                s << (*it).endpoint() << " ";
            }]=]
    [=[            for (auto const & entry : results) {
                s << entry.endpoint() << " ";
            }]=]
)
replace_in_file(
    "websocketpp/transport/asio/endpoint.hpp"
    "                iterator,\n"
    "                results.begin(),\n                results.end(),\n"
)
replace_in_file(
    "websocketpp/transport/asio/endpoint.hpp"
    "->expires_from_now()"
    "->expiry() - lib::asio::steady_timer::clock_type::now()"
)
replace_in_file(
    "websocketpp/transport/asio/endpoint.hpp"
    "        m_io_service->reset();"
    "        m_io_service->restart();"
)

replace_in_file(
    "websocketpp/transport/asio/security/none.hpp"
    "typedef lib::asio::io_service* io_service_ptr;"
    "typedef lib::asio::io_context* io_service_ptr;"
)
replace_in_file(
    "websocketpp/transport/asio/security/none.hpp"
    "typedef lib::shared_ptr<lib::asio::io_service::strand> strand_ptr;"
    "typedef lib::shared_ptr<lib::asio::io_context::strand> strand_ptr;"
)

replace_in_file(
    "websocketpp/transport/asio/security/tls.hpp"
    "typedef lib::asio::io_service * io_service_ptr;"
    "typedef lib::asio::io_context * io_service_ptr;"
)
replace_in_file(
    "websocketpp/transport/asio/security/tls.hpp"
    "typedef lib::shared_ptr<lib::asio::io_service::strand> strand_ptr;"
    "typedef lib::shared_ptr<lib::asio::io_context::strand> strand_ptr;"
)
replace_in_file(
    "websocketpp/transport/asio/security/tls.hpp"
    "m_strand->wrap("
    "lib::asio::bind_executor(*m_strand, "
)
