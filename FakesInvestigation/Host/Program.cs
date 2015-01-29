using System;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Host
{
    class Program
    {
        static void Main(string[] args)
        {
            RegisterProfiler(false);
            RegisterProfiler(true);

            StartTargetProcess();

            UnregisterProfiler(false);
            UnregisterProfiler(true);

            Console.ReadLine();
        }

        private static void RegisterProfiler(bool register64)
        {
            var info = new ProcessStartInfo
            {
                Arguments = string.Format(@"/s /n /i:user {0}\SpikeProfiler.dll", register64 ? "x64" : "x86"),
                UseShellExecute = false,
                FileName = @"regsvr32.exe"
            };

            var process = Process.Start(info);

            process.WaitForExit();
        }

        private static void UnregisterProfiler(bool register64)
        {
            var info = new ProcessStartInfo
            {
                Arguments = string.Format(@"/s /u /n /i:user {0}\SpikeProfiler.dll", register64 ? "x64" : "x86"),
                UseShellExecute = false,
                FileName = @"regsvr32.exe"
            };

            var process = Process.Start(info);

            process.WaitForExit();
        }

        private static void StartTargetProcess()
        {
            var info = new ProcessStartInfo
            {
                Arguments = "TargetTests.dll",
                UseShellExecute = false,
                FileName =
                    @"C:\Program Files (x86)\Microsoft Visual Studio 12.0\Common7\IDE\CommonExtensions\Microsoft\TestWindow\vstest.console.exe"
            };

            info.EnvironmentVariables["Cor_Profiler"] = "{0487FC76-4E1C-41D3-BA20-4F85A1F225C0}";
            info.EnvironmentVariables["Cor_Enable_Profiling"] = "1";
            info.EnvironmentVariables["CorClr_Profiler"] = "{0487FC76-4E1C-41D3-BA20-4F85A1F225C0}";
            info.EnvironmentVariables["CorClr_Enable_Profiling"] = "1";

            var process = Process.Start(info);

            process.WaitForExit();
        }
    }
}
