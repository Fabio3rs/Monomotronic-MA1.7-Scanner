using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows.Forms;

namespace Monomotronic_MA1._7_Scanner
{
    static class Program
    {
        /// <summary>
        /// Ponto de entrada principal para o aplicativo.
        /// </summary>
        [STAThread]
        static void Main()
        {
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            Application.Run(new Form1());
        }
    }
}
