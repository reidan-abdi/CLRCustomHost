using System;
using System.Collections.Generic;
using System.Diagnostics;

namespace TestApplication
{
  class Program
  {
    public static void Main()
    {
      Main(1570000000.ToString());
    }

#pragma warning disable 0028
    static int Main(string arg)
#pragma warning restore 0028 
    {
      var wsSize = long.Parse(arg);
      Console.WriteLine("Received working set size: " + wsSize + "Write something to start");
      Console.ReadLine();

      const int ARRAY_SIZE = 4096;
      const float SAFETY_COEFF = 0.7f;
      const int ITERATIONS = 10;

      var arrays = new List<byte[]>();
      for (long i = 0; i < (SAFETY_COEFF * wsSize)/ARRAY_SIZE; i++)
        arrays.Add(new byte[ARRAY_SIZE]);

      Console.WriteLine("Starting work...");
      var sw = Stopwatch.StartNew();
      for (var i = 0; i < ITERATIONS; i++)
      {
        Console.WriteLine("Outer iterations " + i);
        foreach (var array in arrays)
        {
          var sum = 0;
          foreach (var item in array)
            sum += item;
        }
      }

      sw.Stop();

      Console.WriteLine("Average time per iteration: " + sw.ElapsedMilliseconds / ITERATIONS + "ms");
      Console.WriteLine("Finished work, press [RETURN] to quit.");
      Console.ReadLine();
      return 0;
    }
  }
}
