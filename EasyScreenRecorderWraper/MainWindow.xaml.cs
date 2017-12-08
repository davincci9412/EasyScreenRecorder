using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.Runtime.InteropServices;
using System.Threading;

namespace ScreenRecoder
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    /// 
    /// 
    
    public partial class MainWindow : Window
    {
        [DllImport("ScreenRecoderDLL.dll")]        public extern static void startMicrophone();
        [DllImport("ScreenRecoderDLL.dll")]        public extern static int OnStartCapturing();
        [DllImport("ScreenRecoderDLL.dll")]        public extern static int OnStopCapturing();
        [DllImport("ScreenRecoderDLL.dll")]        public extern static int OnPauseResumeCapturing();

        [DllImport("ScreenRecoderDLL.dll", CallingConvention = CallingConvention.Cdecl)]
        public extern static void SetAudioInput(int fmr);

        [DllImport("ScreenRecoderDLL.dll", CallingConvention = CallingConvention.Cdecl)]
        public extern static void SetAudioVolume(int fmr);

        [DllImport("ScreenRecoderDLL.dll", CallingConvention = CallingConvention.Cdecl)]
        public extern static void SetAudioFrequency(int fmr);

        [DllImport("ScreenRecoderDLL.dll", CallingConvention = CallingConvention.Cdecl)]
        public extern static void SetAudioBites(int fmr);

        [DllImport("ScreenRecoderDLL.dll", CallingConvention = CallingConvention.Cdecl)]
        public extern static void SetAudioType(int fmr);

        [DllImport("ScreenRecoderDLL.dll", CallingConvention = CallingConvention.Cdecl)]
        public extern static void SetAudioFormat(int fmr);

        [DllImport("ScreenRecoderDLL.dll", CallingConvention = CallingConvention.Cdecl)]
        public extern static void SetAudioIS(int fmr);

        [DllImport("ScreenRecoderDLL.dll", CallingConvention = CallingConvention.Cdecl)]
        public extern static void SetVideoWidth(int fmr);

        [DllImport("ScreenRecoderDLL.dll", CallingConvention = CallingConvention.Cdecl)]
        public extern static void SetVideoHeight(int fmr);

        [DllImport("ScreenRecoderDLL.dll", CallingConvention = CallingConvention.Cdecl)]
        public extern static void SetVideoFrameRates(int fmr);

        [DllImport("ScreenRecoderDLL.dll", CallingConvention = CallingConvention.Cdecl)]
        public extern static void SetVideoCodec(int fmr);


        Thread  myThread;
        int     myCaptureState = 0;
        //0: Stop, 1: Starting, 2: Paused


        /// <summary>
        /// 
        public int m_audioinput;//MIC
        public int m_audiovolume;//10
        public int m_audiofrequency;//44100Hz
        public int m_audiobites;//16-bit
        public int m_audiotype;//Stereo, Mono
        public int m_audioformat;//AVI, MP4
        public int m_audioIS;//
        public int m_videowidth;//1440
        public int m_videoheight;//900
        public int m_videoframerates;//25
        public int m_videoCodec;//WMV, MP4


        
        /// </summary>
        public MainWindow()
        {

            InitializeComponent();

            m_audioinput = 0;//MIC
            m_audiovolume = 10;//10
            m_audiofrequency = 44100;//44100Hz
            m_audiobites = 16;//16-bit
            m_audiotype = 0;//Stereo, Mono
            m_audioformat = 1;//AVI, MP4
            m_audioIS = 0;//
            m_videowidth = 1440;//1440
            m_videoheight = 900;//900
            m_videoframerates = 25;//25
            m_videoCodec = 0;//WMV, MP4

        }

        private void configureEngine()
        {
/*          SetAudioInput(m_audioinput);
            SetAudioVolume(m_audiovolume);
            SetAudioFrequency(m_audiofrequency);
            SetAudioBites(m_audiobites);
            SetAudioType(m_audiotype);
            SetAudioFormat(m_audioformat);
*/            
            SetAudioIS(m_audioIS);
            SetVideoWidth(m_videowidth);
            SetVideoHeight(m_videoheight);
            SetVideoFrameRates(m_videoframerates);
            SetVideoCodec(m_videoCodec);
            
        }
        private void OnClick_Start(object sender, RoutedEventArgs e)
        {
            
            if (myCaptureState == 0)
            {
                startMicrophone();
                configureEngine();

                myThread = new Thread(new ThreadStart(Thread_Body));

                myThread.Start();
                BtnStart.Content = "Pause";
                
                myCaptureState = 1;
                BtnStop.IsEnabled = true;

            }
            else if (myCaptureState == 2)
            {
                OnPauseResumeCapturing();
                BtnStart.Content = "Pause";

                myCaptureState = 1;
            }
            else if (myCaptureState == 1)
            {
                OnPauseResumeCapturing();
                BtnStart.Content = "Resume";

                myCaptureState = 2;
            }
            
            BtnConfigure.IsEnabled = false;
        }

        private void OnClick_Stop(object sender, RoutedEventArgs e)
        {
            if (myCaptureState==2)
            {
                OnPauseResumeCapturing();

            }

            OnStopCapturing();
            myThread.Abort();
            
            
            myCaptureState = 0;
            BtnStart.Content = "Start";

            BtnStop.IsEnabled = false;  
            BtnConfigure.IsEnabled = true;
        }

        private void OnClick_Configure(object sender, RoutedEventArgs e)
        {
            Configure myConfigure = new Configure(this);
            myConfigure.Show();
            
        }
        private void OnClick_Exit(object sender, RoutedEventArgs e)
        {
           this.Close();
        }
        public void Thread_Body()
        {
            try
            {
                startMicrophone();
                int result = OnStartCapturing();

            }
            catch (Exception e)
            {

            }
        }
        
    }
}
