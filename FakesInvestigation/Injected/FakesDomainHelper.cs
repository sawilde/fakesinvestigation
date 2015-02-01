using System;

namespace Injected
{
    public class FakesDomainHelper : IFakesDomainHelper
    {
        public void AddResolveEventHandler()
        {
            AppDomain.CurrentDomain.AssemblyResolve += 
                (sender, args) => args.Name.StartsWith("Injected, Version=1.0.0.0") ? System.Reflection.Assembly.GetExecutingAssembly() : null;
        }
    }
}