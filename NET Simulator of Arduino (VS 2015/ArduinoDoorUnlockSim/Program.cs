using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Threading;
using FireSharp.Config;
using FireSharp.Response;
using FireSharp.EventStreaming;
using FireSharp.Interfaces;
using FireSharp;

namespace ArduinoDoorUnlockSim
{
    class Program
    {
        static IFirebaseClient client;
        static string door;

        static void Main(string[] args)
        {
            IFirebaseConfig config = new FirebaseConfig
            {
                AuthSecret = "YOUR_SECRET",
                BasePath = "YOUR_FIREBASE_URL"
            };

            client = new FirebaseClient(config);

            Console.Write("Insert door name: ");
            door = Console.ReadLine();



            Console.WriteLine("***************************************");
            Console.WriteLine("MENU:");
            Console.WriteLine("1. Simulate Arduino");
            Console.WriteLine("2. Clean Log data");
            Console.Write("Insert your choose (1/2): ");
            string izbor = Console.ReadLine();
            Console.WriteLine();

            if (izbor == "1")
            {
                Console.WriteLine("Simulating .....");
                Console.WriteLine("Press any key to finish");

                Dictionary<string, int> data = new Dictionary<string, int>();
                data.Add("unlock", 0);
                client.UpdateAsync(door + "/State/", data);


                Task<EventStreamResponse> task = client.OnAsync(door + "/State", null, FirebaseChanged);

                System.Timers.Timer timer = new System.Timers.Timer();
                timer.Elapsed += Timer_Elapsed;
                timer.Interval = new Random().Next(10000, 50000);
                timer.Enabled = true;

                Console.Read();
                task.Result.Dispose();
            }
            else if (izbor == "2")
            {
                cleanFirebase();
            }

            Console.WriteLine("Program finished");

        }

        private static void cleanFirebase()
        {
            Task <FirebaseResponse> t = client.DeleteAsync(door + "/Log"); //Deletes todos collection
            t.Wait();
        }

        private static void Timer_Elapsed(object sender, System.Timers.ElapsedEventArgs e)
        {
            System.Timers.Timer timer = (System.Timers.Timer)sender;

            Console.WriteLine("Write in Log");
            var myEntityO = new
            {
                opened = new Dictionary<string, string> { { ".sv", "timestamp" } }
            };

           
            Task<PushResponse> t = client.PushAsync(door + "/Log/", myEntityO);
            t.Wait();
            string key = t.Result.Result.name;

            Thread.Sleep(new Random().Next(2000, 10000));
            var myEntityC = new
            {
                closed = new Dictionary<string, string> { { ".sv", "timestamp" } }
            };
            client.UpdateAsync(door + "/Log/" + key, myEntityC);

            timer.Enabled = false;
            timer.Interval = new Random().Next(10000, 50000);
            timer.Enabled = true;
        }

        static void FirebaseChanged(object sender, ValueChangedEventArgs args, object context)
        {
            Console.WriteLine(args.OldData + " --:: " + args.Data);
            if (args.Data == "1")
            {
                Thread.Sleep(2000);
                Dictionary<string, int> data = new Dictionary<string, int>();
                data.Add("unlock", 0);

                client.UpdateAsync(door + "/State/", data);
            }
        }
    }
}
