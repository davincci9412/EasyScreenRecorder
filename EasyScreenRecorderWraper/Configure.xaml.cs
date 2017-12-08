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
using System.Windows.Shapes;

namespace ScreenRecoder
{
    /// <summary>
    /// Interaction logic for Configure.xaml
    /// </summary>
    public partial class Configure : Window
    {
        MainWindow mainwindow;

        int m_audioinput;//MIC
        int m_audiovolume;//10
        int m_audiofrequency;//44100Hz
        int m_audiobites;//16-bit
        int m_audiotype;//Stereo, Mono
        int m_audioformat;//AVI, MP4
        int m_audioIS;//
        int m_videowidth;//1440
        int m_videoheight;//900
        int m_videoframerates;//25
        int m_videoCodec;//WMV, MP4

        public Configure()
        {
            InitializeComponent();
        }
         
        public Configure(MainWindow m)
        {
            mainwindow = m;
            m_audioinput = mainwindow.m_audioinput;//MIC
            m_audiovolume = mainwindow.m_audiovolume;//10
            m_audiofrequency = mainwindow.m_audiofrequency;//44100Hz
            m_audiobites = mainwindow.m_audiobites;//16-bit
            m_audiotype = mainwindow.m_audiotype;//Stereo, Mono
            m_audioformat = mainwindow.m_audioformat;//AVI, MP4
            m_audioIS = mainwindow.m_audioIS;//
            m_videowidth = mainwindow.m_videowidth;//1440
            m_videoheight = mainwindow.m_videoheight;//900
            m_videoframerates = mainwindow.m_videoframerates;//25
            m_videoCodec = mainwindow.m_videoCodec;//WMV, MP4

            InitializeComponent();


            List<string> audioinputdevices = new List<string>();
            audioinputdevices.Add("MIC");
            audioinputdevices.Add("WASAPI-WindowsAudioSessionAPI");
            ComboAudioInputDevice.ItemsSource = audioinputdevices;
            ComboAudioInputDevice.SelectedIndex = m_audioinput;

            List<string> audiobitrates = new List<string>();
            audiobitrates.Add("44100Hz, 16-bit Stereo");
            audiobitrates.Add("22050Hz, 16-bit Stereo");
            audiobitrates.Add("11025Hz, 16-bit Stereo");
            ComboAudioBitrates.ItemsSource = audiobitrates;
            switch(m_audiofrequency){
                case 44100:
                    ComboAudioBitrates.SelectedIndex = 0;
                    break;
                case 22050:
                    ComboAudioBitrates.SelectedIndex = 1;
                    break;
                case 11025:
                    ComboAudioBitrates.SelectedIndex = 2;
                    break;
            }
                    
            List<string> videoresolutions = new List<string>();
            videoresolutions.Add("800 x 600");
            videoresolutions.Add("1280 x 800");
            videoresolutions.Add("1440 x 900");
            videoresolutions.Add("1920 x 1080");
            ComboVideoResolution.ItemsSource = videoresolutions;
            switch (m_videowidth)
            {
                case 800:
                    ComboVideoResolution.SelectedIndex = 0;
                    break;
                case 1280:
                    ComboVideoResolution.SelectedIndex = 1;
                    break;
                case 1440:
                    ComboVideoResolution.SelectedIndex = 2;
                    break;
                case 1920:
                    ComboVideoResolution.SelectedIndex = 3;
                    break;
            }


            List<string> videoframerates = new List<string>();
            videoframerates.Add("20 Frames");
            videoframerates.Add("25 Frames");
            videoframerates.Add("30 Frames");
            ComboVideoFrameRates.ItemsSource = videoframerates;
            switch (m_videoframerates)
            {
                case 20:
                    ComboVideoFrameRates.SelectedIndex = 0;
                    break;
                case 25:
                    ComboVideoFrameRates.SelectedIndex = 1;
                    break;
                case 30:
                    ComboVideoFrameRates.SelectedIndex = 2;
                    break;
            }


            List<string> videocodecs = new List<string>();
            videocodecs.Add("H.264/MPEG-4");
            videocodecs.Add("WMV");
            ComboVideoCodec.ItemsSource = videocodecs;
            ComboVideoCodec.SelectedIndex = m_videoCodec;


            if (m_audioformat == 0){
                OptionAudioAVI.IsChecked = true;
                OptionAudioMP4.IsChecked = false;
            }
            else
            {
                OptionAudioAVI.IsChecked = false;
                OptionAudioMP4.IsChecked = true;

            }

            if (m_audioIS == 1)
            {
                CheckAudioIS.IsChecked = true;
            }
            else
            {
                CheckAudioIS.IsChecked = false;
            }

        }

        public void ComboAudioInputDevice_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            var comboBox = sender as ComboBox;
            switch (comboBox.SelectedIndex)
            {
                case 0:
                    m_audioinput = 0;//"MIC";
                    break;
                case 1:
                    m_audioinput = 1;//"WASAPI";
                    break;
            }
        }

        public void ComboAudioBitrates_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            var comboBox = sender as ComboBox;
            switch (comboBox.SelectedIndex)
            {
                case 0:
                    m_audiofrequency = 44100;
                    m_audiobites = 16;
                    m_audiotype = 0;//"stereo";
                    break;
                case 1:
                    m_audiofrequency = 22050;
                    m_audiobites = 16;
                    m_audiotype = 0;//"stereo";
                    break;
                case 2:
                    m_audiofrequency = 11025;
                    m_audiobites = 16;
                    m_audiotype = 0;//"stereo";
                    break;
            }
        }

        public void ComboVideoResolution_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            var comboBox = sender as ComboBox;
            switch (comboBox.SelectedIndex)
            {
                case 0:
                    m_videowidth = 800;
                    m_videoheight = 600;
                    break;
                case 1:
                    m_videowidth = 1280;
                    m_videoheight = 800;
                    break;
                case 2:
                    m_videowidth = 1440;
                    m_videoheight = 900;
                    break;
                case 3:
                    m_videowidth = 1920;
                    m_videoheight = 1080;
                    break;
            }
        }



        public void ComboVideoFrameRates_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            var comboBox = sender as ComboBox;
            switch (comboBox.SelectedIndex)
            {
                case 0:
                    m_videoframerates = 20;
                    break;
                case 1:
                    m_videoframerates = 25;
                    break;
                case 2:
                    m_videoframerates = 30;
                    break;
            }
        }


        public void ComboVideoCodec_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            var comboBox = sender as ComboBox;
            switch (comboBox.SelectedIndex)
            {
                case 0:
                    m_videoCodec = 0;//"mp4";
                    break;
                case 1:
                    m_videoCodec = 1;//"wmv";
                    break;
            }
        }

        private void RadioButton_AudioAVIChecked(object sender, RoutedEventArgs e)
        {
            m_audioformat = 0;
        }

        private void RadioButton_AudioMP4Checked(object sender, RoutedEventArgs e)
        {
            m_audioformat = 1;
            
        }

        private void CheckButton_AudioIS(object sender, RoutedEventArgs e)
        {
            m_audioIS = 1;
        }
        private void CheckButton_AudioISno(object sender, RoutedEventArgs e)
        {
            m_audioIS = 0;
        }
        private void OnClick_OK(object sender, RoutedEventArgs e)
        {
            mainwindow.m_audioinput = m_audioinput;//MIC
            mainwindow.m_audiovolume = m_audiovolume;//10
            mainwindow.m_audiofrequency = m_audiofrequency;//44100Hz
            mainwindow.m_audiobites = m_audiobites;//16-bit
            mainwindow.m_audiotype = m_audiotype;//Stereo, Mono
            mainwindow.m_audioformat = m_audioformat;//AVI, MP4
            mainwindow.m_audioIS = m_audioIS;//
            mainwindow.m_videowidth = m_videowidth;//1440
            mainwindow.m_videoheight = m_videoheight;//900
            mainwindow.m_videoframerates = m_videoframerates;//25
            mainwindow.m_videoCodec = m_videoCodec;//WMV, MP4
            this.Close();
        }
        private void OnClick_Cancel(object sender, RoutedEventArgs e)
        {
            this.Close();
        }
    }
}
