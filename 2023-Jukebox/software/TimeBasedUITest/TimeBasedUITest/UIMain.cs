using System;
using System.Drawing;
using System.Threading;
using System.Windows.Forms;

namespace TimeBasedUITest
{
    public partial class UIMain : Form
    {
        public UIMain()
        {
            InitializeComponent();

            FormClosing += UIMain_FormClosing;
        }

        private void UIMain_FormClosing(object sender, FormClosingEventArgs e)
        {
            Environment.Exit(0);
        }

        private void StartAnimation()
        {
            while (true)
            {
                Invoke((MethodInvoker)delegate {
                    UpdateAnimation();
                });
               
                Thread.Sleep(100);
            }
        }

        private int Constrain(int x, int min, int max)
        {
            return Math.Min(Math.Max(x, min), max);
        }

        private int Cube(double x)
        {
            return (int)(x * x * x / 255.0 / 255.0);
        }

        private void UpdateAnimation()
        {
            Button[] dots = { btnDot0, btnDot1, btnDot2, btnDot3, btnDot4, btnDot5 };
            Button[] bars = { btnBar0, btnBar1, btnBar2, btnBar3, btnBar4 };
            string lampCode = "--- ---  - ---  ---  - - - -   ";

            double dotIndex = Math.Sin(Millis() / 10 * Math.PI / 180) * 2.5 + 2.5;
            for (int i = 0; i < 6; i++)
            {
                double brightness = 255.0 - 51.0 * Math.Abs(i - dotIndex);
                dots[i].BackColor = Color.FromArgb(Constrain(Cube(brightness), 0, 255), Color.White);
            }

            double barIndex = Math.Sin(Millis() / 10 * Math.PI / 180) * 2.0 + 2.0;
            for (int i = 0; i < 5; i++)
            {
                double brightness = 255.0 - 63.75 * Math.Abs(i - barIndex);
                bars[i].BackColor = GetColor(Constrain(Cube(brightness), 0, 255));
            }

            long lampCodeIndex = Millis() / 200 % lampCode.Length;
            btnLamp.BackColor = Color.FromArgb(lampCode[(int)lampCodeIndex] == '-' ? 255 : 0, Color.Green);
            btnLamp.Text = lampCodeIndex.ToString();
        }

        private Color GetColor(int alpha)
        {
            double colorIndex = Math.Sin(Millis() / 20 * Math.PI / 180) + 1;
            int r = Cube(255.0 - 85.0 * Math.Abs(0 - colorIndex));
            int b = Cube(255.0 - 85.0 * Math.Abs(1 - colorIndex));
            int w = Cube(255.0 - 85.0 * Math.Abs(2 - colorIndex));
            return Color.FromArgb(alpha, (r + w) / 2, w, (b + w) / 2);
        }

        private long Millis()
        {
            return DateTime.Now.Ticks / TimeSpan.TicksPerMillisecond;
        }

        private void UIMain_Load(object sender, EventArgs e)
        {
            new Thread(StartAnimation).Start();
        }
    }
}
