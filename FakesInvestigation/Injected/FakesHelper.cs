using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Microsoft.Win32;

namespace Injected
{
    public class FakesHelper
    {
        private const string COR_ENABLE_PROFILING = "COR_ENABLE_PROFILING";
        private const string COR_PROFILER = "COR_PROFILER";
        private const string CHAIN_EXTERNAL_PROFILER = "CHAIN_EXTERNAL_PROFILER";
        private const string CHAIN_EXTERNAL_PROFILER_LOCATION = "CHAIN_EXTERNAL_PROFILER_LOCATION";

        public static void Log(object data)
        {
            var dict = data as IDictionary<string, string>;
            if (dict == null) return;
            foreach (var entry in dict)
            {
                Console.WriteLine("{0}:{1}", entry.Key, entry.Value);
            }
        }

        public static void LoadProfilerInstead(object data)
        {
            var dict = data as IDictionary<string, string>;
            if (dict == null) return;

            if (!dict.ContainsKey(COR_ENABLE_PROFILING) || dict[COR_ENABLE_PROFILING] != "1") return;

            dict[CHAIN_EXTERNAL_PROFILER] = dict[COR_PROFILER];
            dict[COR_PROFILER] = "{0487FC76-4E1C-41D3-BA20-4F85A1F225C0}";

            var key = Registry.ClassesRoot.OpenSubKey(string.Format("CLSID\\{0}\\InprocServer32", 
                dict[CHAIN_EXTERNAL_PROFILER]));

            if (key == null) return;
            var location = key.GetValue(null) as string;
            dict[CHAIN_EXTERNAL_PROFILER_LOCATION] = location;
        }

        public static void PretendWeLoadedFakesProfiler(object data)
        {
            var args = data as string[];
            foreach (var arg in args ?? new string[0])
            {
                Console.WriteLine(arg);
            }

            var enabled = Environment.GetEnvironmentVariable(COR_ENABLE_PROFILING);
            if (enabled == "1")
            {
                Environment.SetEnvironmentVariable(COR_PROFILER, "{44250666-1751-4368-a29c-31caf4ccf3f5}");
            }
        }
    }
}
