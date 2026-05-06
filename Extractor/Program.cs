using System;
using System.IO;
using System.Reflection;

class Program
{
    static void Main(string[] args)
    {
        try
        {
            var asm = Assembly.LoadFile(Path.GetFullPath("../Assets/Mods/Autumn/Autumn.dll"));
            Console.WriteLine("Loaded Assembly: " + asm.FullName);
            foreach (var res in asm.GetManifestResourceNames())
            {
                Console.WriteLine("Resource: " + res);
                using (var stream = asm.GetManifestResourceStream(res))
                using (var file = File.Create(res))
                {
                    stream.CopyTo(file);
                }
            }
        }
        catch (Exception e)
        {
            Console.WriteLine(e.ToString());
        }
    }
}
