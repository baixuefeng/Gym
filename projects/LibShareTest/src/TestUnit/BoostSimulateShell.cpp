#include "stdafx.h"
#include "TestUnit/BoostSimulateShell.h"
#include <string>
#include <type_traits>
#include <boost/log/trivial.hpp>
#include <boost/process.hpp>

void SimulateShell()
{
    boost::asio::io_context ioDevice;
    boost::process::async_pipe in{ioDevice};
    boost::process::async_pipe out{ioDevice};
    boost::process::child subProcess{boost::process::exe = boost::process::shell(),
                                     boost::process::std_in<in, boost::process::std_out> out};
    boost::asio::async_write(in,
                             boost::asio::buffer("dir\nexit\n"),
                             [](const boost::system::error_code &, std::size_t) {});
    std::string output;
    output.assign(10000, 0);

    std::function<void(const boost::system::error_code &, size_t)> pf;
    pf = std::function<void(const boost::system::error_code &, size_t)>(
        [&output, &pf, &out](const boost::system::error_code &err, std::size_t c) {
            BOOST_LOG_TRIVIAL(info) << err.message() << "  " << c << "\n";
            if (!err)
            {
                output.erase(c);
                BOOST_LOG_TRIVIAL(info) << output;
                output.assign(10000, 0);
                out.async_read_some(boost::asio::buffer(output), pf);
            }
        });

    out.async_read_some(boost::asio::buffer(output), pf);

    ioDevice.run();
    subProcess.wait();
}
