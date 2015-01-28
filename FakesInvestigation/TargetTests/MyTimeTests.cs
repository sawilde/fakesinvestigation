using System;
using Microsoft.QualityTools.Testing.Fakes;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Target;

namespace TargetTests
{
    [TestClass]
    public class MyTimeTests
    {
        [TestMethod]
        public void GetTheCurrentYear_When_2016()
        {
            // Shims can be used only in a ShimsContext:
            using (ShimsContext.Create())
            {
                // Arrange:
                // Shim DateTime.Now to return a fixed date:
                System.Fakes.ShimDateTime.NowGet =
                  () =>
                  { return new DateTime(2016, 1, 1); };

                Assert.AreEqual(2016, (new MyTime()).GetTheCurrentYear());
            }
        }
    }
}
