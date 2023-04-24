#!/usr/bin/env python3
# -*- coding: UTF-8 -*-

from pathlib import Path
import platform
from pubsub import pub
from shutil import rmtree
import subprocess
from threading import Thread
import wx
from wx.lib.embeddedimage import PyEmbeddedImage


repo = ('kinsamanka/openplc-uploader/archive/refs/heads/master',
        'kinsamanka/matiec/archive/20280846',
        'Jacajack/liblightmodbus/archive/3f8ebeba')

dir_names = ('openplc-uploader-master',
             'matiec-2028084698ed81594da3d4f19b81fb17ffd4c71b',
             'liblightmodbus-3f8ebeba2d14767c12f502025844463c6a04ecba')


class WorkerThread(Thread):

    def __init__(self, cmd):
        self.cmd = cmd
        Thread.__init__(self)
        self.start()

    def run(self):
        process = subprocess.Popen(
            self.cmd,
            encoding='utf-8',
            errors='replace',
            shell=True,
            stderr=subprocess.STDOUT,
            stdout=subprocess.PIPE,
        )

        out = {'busy': True, 'data': [], 'index': -1}

        while True:
            r = process.stdout.readline()
            if r == '' and process.poll() is not None:
                break
            if r:
                out['index'] += 1
                out['data'].append(r.strip())
                wx.CallAfter(pub.sendMessage, "console_output", e=out)

        out['busy'] = False
        wx.CallAfter(pub.sendMessage, "console_output", e=out)


class Console(wx.Frame):

    _logo = PyEmbeddedImage(
        b'iVBORw0KGgoAAAANSUhEUgAAAEAAAABACAYAAACqaXHeAAAABmJLR0QA/wD/AP+gvaeTAAAC'
        b'sklEQVR4nO2av2sUQRTHPyfxEoU0BsGIZSqL1CramULPxhSWCqJ/gGITO/8DY21lq4JibeMP'
        b'MNEUsRaxELURDDa5RHIW8za3bu7W7M7Me8vdfGBYbmbn7fe9ezczOzeQMGMaWJEybaxFnRbw'
        b'GOhJeQYcMFWkzD2c4z+l9KRuLLgM7AB/gAvAeWBb6q4Y6lLhJLCB+8Zv5epvS91vYN5AlwpH'
        b'gE84Rx8NaH8obV+Ao4q6VJgAXuIcXAMODbhnCngn97wC2mrqFHiAc+w7cKLkvlngq9y7rKBL'
        b'has4h7aAc/u4/xSwKX1uRtSlwmn6ztyo0K9q0BpJPp3v1+i/zP5+No1kCrfEzQa0gzVsFAfO'
        b'w8HUDeESsAp06S9RfctnYMZD04zYCKWnKz52Bjkf6iFZCbWomRdbofX9E4T3UrmE3/y7gFvi'
        b'7gCLHnaKLNJfPi942GkDd3G+ruQbsrT3cX6OuC82+ReoOQ87k2JnM1+ZpYUPawxPt9c17L0t'
        b'sffBU+uuvyHfwX+VtJ2tYe9MSdtGDXulhMiA0LZVNI3VLswgUgCsBViTAmAtwJoUAGsB1qQA'
        b'WAuwJgXAWoA1KQDWAqxJAbAWYE0KgLUAayYUnxVrd8cLjQx449G3zmZqLWLuvzUR1T3Bsu3t'
        b'/5XoGdCSa6/wOSS+mRVTU0tzEKzqiMpPMs0CCs9Is0ADSf8MZRTHgHHKAiBlwJ4MiDHnNpHd'
        b'TB/7DMgCsCXXkTqfO4RJuXahH4CPcr3DaAehjfMRYD3f0CH8MbSml4vF6HRwR8dCHpRsWumK'
        b'j3ucj8lTefhShT7ZWb4nURQpcgw3wG4DxxX6VSb2NHgdd1D6OfCtQr8fwAvcOuVaBF3B8dn1'
        b'qVuCvCmGWvn1Atmpirf+0BsiWkvpYAEPHQCrTKhNqEHQZ9enLmq7RSPNX2LziSk36i6vAAAA'
        b'AElFTkSuQmCC')

    def __init__(self, cmd=''):
        super().__init__(None, title="Updating ...", size=(600, 300))

        self.SetIcon(self._logo.GetIcon())

        self.panel_1 = wx.Panel(self, wx.ID_ANY)

        sizer_1 = wx.FlexGridSizer(2, 1, 0, 0)

        self.text_ctrl_1 = wx.TextCtrl(
            self.panel_1,
            wx.ID_ANY,
            '',
            style=wx.HSCROLL | wx.VSCROLL | wx.TE_MULTILINE | wx.TE_READONLY)
        self.text_ctrl_1.SetFont(wx.Font(8, wx.FONTFAMILY_TELETYPE, wx.NORMAL,
                                         wx.NORMAL, faceName="Monospace"))
        self.text_ctrl_1.SetForegroundColour(wx.Colour(0xF0, 0xF0, 0xF0))
        self.text_ctrl_1.SetBackgroundColour(wx.Colour(0, 0, 0))

        sizer_1.Add(self.text_ctrl_1, 0, wx.EXPAND, 0)

        sizer_1.AddGrowableRow(0)
        sizer_1.AddGrowableCol(0)
        self.panel_1.SetSizer(sizer_1)

        self.Layout()

        self.output_index = -1
        pub.subscribe(self.on_result, "console_output")

        self.worker = WorkerThread(cmd)

    def on_result(self, e):
        while self.output_index < e['index']:
            self.output_index += 1
            self.text_ctrl_1.WriteText('%s\n' % e['data'][self.output_index])
        if not e['busy']:
            self.worker = None
            self.Close()


class MyApp(wx.App):
    def __init__(self, *pargs, cmd=None, **kwargs):
        self.cmd = cmd
        super().__init__(*pargs, **kwargs)

    def OnInit(self):
        self.frame = Console(cmd=self.cmd)
        self.SetTopWindow(self.frame)
        self.frame.Show()
        return True


is_windows = platform.system() == "Windows"

sep = ' & ' if is_windows else ' ; '


def p7zip_init(cwd, path):
    p = Path(path)

    curl = f'"{p / "bin" / "curl"}"'
    p7z = f'"{p / "bin" / "7z"}"'

    curl += f' --cacert "{p / "ssl" / "cacert.pem"}"'

    cmd = ''
    for r in repo:
        cmd += (f'{curl} -L https://github.com/{r}.tar.gz | '
                f'{p7z} x -tgzip -si -so | '
                f'{p7z} x -ttar -si -y -o"{Path(cwd).parent / "tmp"}"')
        cmd += sep
    return cmd


def tar_init(cwd, path):
    p = Path(path)
    c = Path(cwd)

    curl = f'"{p / "bin" / "curl"}"'
    tar = f'"{p / "bin" / "tar"}"'

    curl += f' --cacert "{p / "ssl" / "cacert.pem"}"'

    cmd = (f'{curl} -L https://github.com/{repo[0]}.tar.gz | '
           f'{tar} xvzf - --strip=1 -C "{c}"')
    cmd += sep
    cmd += (f'{curl} -L https://github.com/{repo[1]}.tar.gz | '
            f'{tar} xvzf - --strip=1 -C "{c / "lib" / "matiec"}"')
    cmd += sep
    cmd += (f'{curl} -L https://github.com/{repo[2]}.tar.gz | '
            f'{tar} xvzf - --strip=1 -C "{c / "lib" / "modbus"}"')

    return cmd


def main(cwd=None, path=None, update=False):

    if cwd and path:
        if is_windows:
            rmtree(Path(cwd), ignore_errors=True)
            cmd = p7zip_init(cwd, path)
        else:
            Path(cwd).mkdir(parents=True, exist_ok=True)
            cmd = tar_init(cwd, path)

    else:
        cmd = f'echo "no supplied commands" {sep} sleep 10'

    app = MyApp(cmd=cmd)
    app.MainLoop()

    if cwd and path and is_windows:
        dst = Path(cwd)
        src = dst.parent / 'tmp'

        (src / dir_names[0]).rename(dst)

        rmtree(dst / 'lib' / 'matiec', ignore_errors=True)
        rmtree(dst / 'lib' / 'modbus', ignore_errors=True)

        (src / dir_names[1]).rename(dst / 'lib' / 'matiec')
        (src / dir_names[2]).rename(dst / 'lib' / 'modbus')

        rmtree(src, ignore_errors=True)


if __name__ == "__main__":
    main()
