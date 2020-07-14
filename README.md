# FreeSWITCH

本仓库是 https://xswith.cn 使用的FreeSWITCH仓库，在官方仓库的基础上，加了一些有用的补丁。

多年来，xswitch.cn 的开发者向FreeSWITCH开源项目贡献了数不清的补丁和新特性，并且，我们也会一如即往地贡献。

并不是我们所有的补丁都可以被上游合并，但是，我们相信有一些补丁对大家也是有用的，所以，我们维护了这个仓库。

本仓库并不想另立山头，我们所有的修改，都会基于上游的修改，我们也会不定期地将上游的修改合并进来。为了能最大限度地与上游仓库兼容，我们使用`rebase`以及`push -f`的方式维护代码，这意味着，我们会不定期地重写提交历史。所以，如果你曾经克隆过本仓库里的代码，又想保持更新的话，几乎一定要把`git pull`换成`git pull --rebase`。请花一些时间学习Git这个有用的工具，我们也会在合适的时间写一份更详细的指南。

本仓库以Github为主，在Github和XSwitch Git上接受PR，Gitee仅是一个镜像。我们也会精选出一些PR推到上游去，或者，会推荐你自己到上游去提PR。

* Github地址：https://github.com/rts-cn/xswitch
* XSwitch Git地址：https://git.xswitch.cn/xswitch/xswitch
* 国内镜象地址：https://gitee.com/rts-cn/xswitch
* 官方仓库的地址：https://github.com/signalwire/freeswitch

其实提个PR很简单，参考杜老师提的PR以及相关补丁：

- https://github.com/signalwire/freeswitch/pull/1399/files
- https://github.com/signalwire/freeswitch/commits?author=seven1240

感谢Anthony Minessale创造了这么优秀的项目，感谢所有为FreeSWITCH代码和文档做出贡献的人。

如果您有任何问题，都可以在这里提个Issue或到 http://www.freeswitch.org.cn/ 找到我们，一个不行就提两个。

This repo is used in https://xswitch.cn , with some patches.

Over the years, developers at xswitch.cn contributed numerouse patches and features to the FreeSWITCH opensource project, and will still do that as always.

Not all our patches could be accepted by the upstream, but we still think some of those are useful, so here is this repo.

We **DON'T** mean to make a Fork of FreeSWITCH. **ALL** our changes are and will be rebased on the upstream master, and we'll constantly update code from the upstream. Rebase and `push -f` will *rewrite* commit history, so you might always want to use `git pull --rebase` if you want to update code from this repo. Spend some time and learn more Git, we might write more details about how to use `rebase` and this repo.

We accept PRs. And we might also push your PR to upstream, or recommend you to do that yourself.

* Github Repo：https://github.com/rts-cn/freeswitch
* Gitee Mirror：https://gitee.com/rts-cn/freeswitch
* Upstream：https://github.com/signalwire/freeswitch

Thanks Anthony Minessale made this great project, and thanks all who contributed to code and docs made this project better.

Open an issue if you have any question.

---

以下是原来的README。The Original README Follows.

---


FreeSWITCH is a Software Defined Telecom Stack enabling the digital transformation from proprietary telecom switches to a versatile software implementation that runs on any commodity hardware. From a Raspberry PI to a multi-core server, FreeSWITCH can unlock the telecommunications potential of any device. Combined with our hosted cloud platform, SignalWire, FreeSWITCH can interconnect with the outside world and scale to any size.

Visit  [https://signalwire.com](https://signalwire.com/)  or https://github.com/signalwire for more info.

## Getting Started

FreeSWITCH is available on [Github](https://github.com/signalwire/freeswitch) in source code format.  You can checkout the development branch and build for many popular platforms including Linux, Windows, MacOSX and BSD.  There is an issue tracker and pull request system available as part of the repo online.

See [https://freeswitch.com/#getting-started](https://freeswitch.com/#getting-started) for more detailed instructions.

## Additional Help

If you need assistance or have an interest in using a commercially supported build, you can contact coreteam@freeswitch.com to learn about professional services to support your project.

## Voice-over-IP services - SIP / SMS - App Integrations

[SignalWire](https://signalwire.com) is the primary sponsor of the FreeSWITCH project and was founded by the original developers of FreeSWITCH. SignalWire provides scalable services to enhance and scale your project such as SMS, SIP, Serverless Application hosting as well as programmable telecom. mod_signalwire which is distributed in this code base allows you to instantly pair with SignalWire and extend your FreeSWITCH.

## Documentation

The main index for documentation is available at:

  * https://freeswitch.org/confluence/

### Release notes:

  * https://freeswitch.org/confluence/display/FREESWITCH/Release+Notes

### Installation

Step by step tutorials to install FreeSWITCH from packages or source code are available at:

  * [Debian 10 Buster](https://freeswitch.org/confluence/display/FREESWITCH/Debian+10+Buster) [<b>Recommended</b>]
  * [Raspberry Pi](https://freeswitch.org/confluence/display/FREESWITCH/Raspberry+Pi)
  * [CentOS 7](https://freeswitch.org/confluence/display/FREESWITCH/CentOS+7+and+RHEL+7)

## Downloads

  * [Tarballs](https://files.freeswitch.org/releases/freeswitch/)
  * [Windows Installer](http://files.freeswitch.org/windows/installer/x64/)
  * [Windows Sound Packages](http://files.freeswitch.org/windows/installer/x64/sounds/)

## Contributions

GitHub pull requests are the recommended way to contribute to the FreeSWITCH source code:

  * https://github.com/signalwire/freeswitch/pulls

## Community

Slack is our chat system where the developers, the FreeSWITCH team, and the most active users are present.
This is the place to get answers faster and chat with other users in real time. All you need to do is enter your email and verify it on the Slack signup page and you are ready to join in the discussion!

Slack Community:
  * https://signalwire.community/

Mailing list:

  * http://lists.freeswitch.org/mailman/listinfo/freeswitch-users

**Thank you for using FreeSWITCH!**
