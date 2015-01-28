using System;
using System.Collections.Generic;
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

            var process = Process.Start(new ProcessStartInfo
            {
                Arguments = "TargetTests.dll",
                UseShellExecute = false,
                FileName = @"C:\Program Files (x86)\Microsoft Visual Studio 12.0\Common7\IDE\CommonExtensions\Microsoft\TestWindow\vstest.console.exe"
            });

            process.WaitForExit();

            Console.ReadLine();

        }
    }
}
