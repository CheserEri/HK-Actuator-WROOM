import 'dart:async';
import 'dart:convert';

import 'package:flutter/material.dart';
import 'package:http/http.dart' as http;

void main() {
  runApp(const ControlApp());
}

class ControlApp extends StatelessWidget {
  const ControlApp({super.key});

  @override
  Widget build(BuildContext context) {
    final colorScheme = ColorScheme.fromSeed(
      seedColor: const Color(0xFF0B6E4F),
      brightness: Brightness.light,
    );

    return MaterialApp(
      title: 'HK 执行器控制台',
      debugShowCheckedModeBanner: false,
      theme: ThemeData(
        colorScheme: colorScheme,
        scaffoldBackgroundColor: const Color(0xFFF6F3EA),
        useMaterial3: true,
        inputDecorationTheme: const InputDecorationTheme(
          border: OutlineInputBorder(),
        ),
      ),
      home: const ControlDashboard(),
    );
  }
}

class ControlDashboard extends StatefulWidget {
  const ControlDashboard({super.key});

  @override
  State<ControlDashboard> createState() => _ControlDashboardState();
}

class _ControlDashboardState extends State<ControlDashboard> {
  final TextEditingController _hostController =
      TextEditingController(text: 'http://192.168.31.108');
  final TextEditingController _positionAngleController =
      TextEditingController(text: '90');
  final TextEditingController _positionDurationController =
      TextEditingController(text: '800');
  final TextEditingController _pressAngleController =
      TextEditingController(text: '90');
  final TextEditingController _pressDurationController =
      TextEditingController(text: '500');
  final TextEditingController _taskAngleController =
      TextEditingController(text: '90');
  final TextEditingController _taskCountController =
      TextEditingController(text: '1');
  final TextEditingController _taskDurationController =
      TextEditingController(text: '800');

  Timer? _pollTimer;
  DeviceStatus? _status;
  bool _sendingPosition = false;
  bool _sendingPress = false;
  bool _sendingTask = false;
  String _feedback = '已连接控制台，请先刷新设备状态。';

  @override
  void initState() {
    super.initState();
    _pollTimer = Timer.periodic(const Duration(seconds: 2), (_) {
      _fetchStatus(silent: true);
    });
    unawaited(_fetchStatus());
  }

  @override
  void dispose() {
    _pollTimer?.cancel();
    _hostController.dispose();
    _positionAngleController.dispose();
    _positionDurationController.dispose();
    _pressAngleController.dispose();
    _pressDurationController.dispose();
    _taskAngleController.dispose();
    _taskCountController.dispose();
    _taskDurationController.dispose();
    super.dispose();
  }

  Uri _buildUri(String path) {
    final base = _hostController.text.trim().replaceAll(RegExp(r'/$'), '');
    return Uri.parse('$base$path');
  }

  bool _ensureHostProvided() {
    if (_hostController.text.trim().isNotEmpty) {
      return true;
    }

    setState(() {
      _feedback = '请先输入设备地址，例如 http://192.168.31.108';
    });
    return false;
  }

  int? _parseAngle(String raw) {
    final value = int.tryParse(raw.trim());
    if (value == null || value < 0 || value > 180) {
      return null;
    }
    return value;
  }

  int? _parsePositiveInt(String raw) {
    final value = int.tryParse(raw.trim());
    if (value == null || value <= 0) {
      return null;
    }
    return value;
  }

  Future<void> _fetchStatus({bool silent = false}) async {
    if (!_ensureHostProvided()) {
      return;
    }

    final client = http.Client();
    try {
      final response = await client
          .get(_buildUri('/status'))
          .timeout(const Duration(seconds: 5));
      if (response.statusCode != 200) {
        throw Exception('状态请求失败：${response.statusCode}');
      }

      final status = DeviceStatus.fromJson(
        jsonDecode(response.body) as Map<String, dynamic>,
      );
      if (!mounted) {
        return;
      }

      setState(() {
        _status = status;
        if (!silent) {
          _feedback = '已同步设备 ${status.deviceId} 的最新状态。';
        }
      });
    } catch (error) {
      if (!mounted || silent) {
        return;
      }
      setState(() {
        _feedback = '获取状态失败：$error';
      });
    } finally {
      client.close();
    }
  }

  Future<void> _sendJsonCommand({
    required String path,
    required Map<String, Object> body,
    required void Function(bool value) setSending,
    required String sendingMessage,
    required String successMessage,
  }) async {
    if (!_ensureHostProvided()) {
      return;
    }

    setState(() {
      setSending(true);
      _feedback = sendingMessage;
    });

    final client = http.Client();
    try {
      final response = await client
          .post(
            _buildUri(path),
            headers: const {'Content-Type': 'application/json'},
            body: jsonEncode(body),
          )
          .timeout(const Duration(seconds: 5));

      final decoded = jsonDecode(response.body) as Map<String, dynamic>;
      final accepted = decoded['accepted'] == true;
      final message = decoded['message']?.toString() ?? '无返回信息';

      if (!mounted) {
        return;
      }

      setState(() {
        _feedback = accepted
            ? '$successMessage 请求编号 #${decoded['requestId']}。'
            : '指令被拒绝：$message';
      });

      await _fetchStatus(silent: true);
    } catch (error) {
      if (!mounted) {
        return;
      }
      setState(() {
        _feedback = '发送指令失败：$error';
      });
    } finally {
      client.close();
      if (mounted) {
        setState(() {
          setSending(false);
        });
      }
    }
  }

  Future<void> _sendPositionCommand() async {
    final angle = _parseAngle(_positionAngleController.text);
    final durationMs = _parsePositiveInt(_positionDurationController.text);

    if (angle == null) {
      setState(() {
        _feedback = '目标角度必须是 0 到 180 的整数。';
      });
      return;
    }

    if (durationMs == null) {
      setState(() {
        _feedback = '定位执行时间必须是正整数毫秒。';
      });
      return;
    }

    await _sendJsonCommand(
      path: '/position',
      body: {
        'angle': angle,
        'moveDurationMs': durationMs,
        if (_status != null) 'deviceId': _status!.deviceId,
      },
      setSending: (value) => _sendingPosition = value,
      sendingMessage: '正在发送定位指令...',
      successMessage: '舵机开始移动并保持目标角度。',
    );
  }

  Future<void> _sendTaskCommand() async {
    final angle = _parseAngle(_taskAngleController.text);
    final count = _parsePositiveInt(_taskCountController.text);
    final durationMs = _parsePositiveInt(_taskDurationController.text);

    if (angle == null) {
      setState(() {
        _feedback = '任务角度必须是 0 到 180 的整数。';
      });
      return;
    }

    if (count == null) {
      setState(() {
        _feedback = '往返次数必须是正整数。';
      });
      return;
    }

    if (durationMs == null) {
      setState(() {
        _feedback = '任务执行时间必须是正整数毫秒。';
      });
      return;
    }

    await _sendJsonCommand(
      path: '/actuate',
      body: {
        'angle': angle,
        'count': count,
        'moveDurationMs': durationMs,
        if (_status != null) 'deviceId': _status!.deviceId,
      },
      setSending: (value) => _sendingTask = value,
      sendingMessage: '正在发送往返任务...',
      successMessage: '往返任务已开始执行。',
    );
  }

  Future<void> _sendPressCommand() async {
    final angle = _parseAngle(_pressAngleController.text);
    final durationMs = _parsePositiveInt(_pressDurationController.text);

    if (angle == null) {
      setState(() {
        _feedback = '按钮角度必须是 0 到 180 的整数。';
      });
      return;
    }

    if (durationMs == null) {
      setState(() {
        _feedback = '按钮执行时间必须是正整数毫秒。';
      });
      return;
    }

    await _sendJsonCommand(
      path: '/press',
      body: {
        'angle': angle,
        'moveDurationMs': durationMs,
        if (_status != null) 'deviceId': _status!.deviceId,
      },
      setSending: (value) => _sendingPress = value,
      sendingMessage: '正在发送按钮指令...',
      successMessage: '按钮动作已开始，舵机会按下后自动松开。',
    );
  }

  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);
    final currentAngle = _status?.currentAngle ?? 0;
    final wide = MediaQuery.sizeOf(context).width >= 960;

    return Scaffold(
      body: Container(
        decoration: const BoxDecoration(
          gradient: LinearGradient(
            colors: [Color(0xFFF6F3EA), Color(0xFFDDEFE8)],
            begin: Alignment.topLeft,
            end: Alignment.bottomRight,
          ),
        ),
        child: SafeArea(
          child: Center(
            child: ConstrainedBox(
              constraints: const BoxConstraints(maxWidth: 1180),
              child: Padding(
                padding: const EdgeInsets.all(20),
                child: Column(
                  children: [
                    _SectionCard(
                      accent: const Color(0xFF12372A),
                      child: Row(
                        crossAxisAlignment: CrossAxisAlignment.start,
                        children: [
                          Expanded(
                            child: Column(
                              crossAxisAlignment: CrossAxisAlignment.start,
                              children: [
                                Text(
                                  'HK 执行器控制台',
                                  style: theme.textTheme.headlineMedium?.copyWith(
                                    fontWeight: FontWeight.w800,
                                    color: const Color(0xFF12372A),
                                  ),
                                ),
                                const SizedBox(height: 8),
                                Text(
                                  '可以直接设定舵机角度并保持，也可以按指定次数和执行时间执行往返任务。',
                                  style: theme.textTheme.bodyLarge?.copyWith(
                                    color: const Color(0xFF38534A),
                                  ),
                                ),
                                const SizedBox(height: 20),
                                TextField(
                                  controller: _hostController,
                                  decoration: const InputDecoration(
                                    labelText: '设备地址',
                                    hintText: 'http://192.168.31.108',
                                  ),
                                ),
                              ],
                            ),
                          ),
                          const SizedBox(width: 20),
                          _AngleBadge(angle: currentAngle),
                        ],
                      ),
                    ),
                    const SizedBox(height: 20),
                    Expanded(
                      child: Flex(
                        direction: wide ? Axis.horizontal : Axis.vertical,
                        crossAxisAlignment: CrossAxisAlignment.start,
                        children: [
                          Expanded(
                            flex: 3,
                            child: Column(
                              children: [
                                _SectionCard(
                                  child: _ControlSection(
                                    title: '定位模式',
                                    description: '移动到指定角度后保持当前位置，不自动回零。',
                                    child: Column(
                                      children: [
                                        Row(
                                          children: [
                                            Expanded(
                                              child: TextField(
                                                controller:
                                                    _positionAngleController,
                                                keyboardType:
                                                    TextInputType.number,
                                                decoration:
                                                    const InputDecoration(
                                                  labelText: '目标角度',
                                                  hintText: '0-180',
                                                ),
                                              ),
                                            ),
                                            const SizedBox(width: 12),
                                            Expanded(
                                              child: TextField(
                                                controller:
                                                    _positionDurationController,
                                                keyboardType:
                                                    TextInputType.number,
                                                decoration:
                                                    const InputDecoration(
                                                  labelText: '执行时间',
                                                  hintText: '毫秒，例如 800',
                                                ),
                                              ),
                                            ),
                                          ],
                                        ),
                                        const SizedBox(height: 16),
                                        Slider(
                                          min: 0,
                                          max: 180,
                                          divisions: 180,
                                          label: _positionAngleController.text,
                                          value: ((double.tryParse(
                                                            _positionAngleController
                                                                .text) ??
                                                        90.0)
                                                    .clamp(0.0, 180.0))
                                              .toDouble(),
                                          onChanged: (value) {
                                            setState(() {
                                              _positionAngleController.text =
                                                  value.round().toString();
                                            });
                                          },
                                        ),
                                        const SizedBox(height: 8),
                                        Align(
                                          alignment: Alignment.centerLeft,
                                          child: FilledButton(
                                            onPressed: _sendingPosition
                                                ? null
                                                : _sendPositionCommand,
                                            child: Text(
                                              _sendingPosition
                                                  ? '发送中...'
                                                  : '移动并保持',
                                            ),
                                          ),
                                        ),
                                      ],
                                    ),
                                  ),
                                ),
                                const SizedBox(height: 20),
                                _SectionCard(
                                  child: _ControlSection(
                                    title: '按钮模式',
                                    description: '模拟人手按一下开关，按下到目标角度后自动松开回初始角度。',
                                    child: Column(
                                      children: [
                                        Row(
                                          children: [
                                            Expanded(
                                              child: TextField(
                                                controller: _pressAngleController,
                                                keyboardType:
                                                    TextInputType.number,
                                                decoration:
                                                    const InputDecoration(
                                                  labelText: '按钮角度',
                                                  hintText: '0-180',
                                                ),
                                              ),
                                            ),
                                            const SizedBox(width: 12),
                                            Expanded(
                                              child: TextField(
                                                controller:
                                                    _pressDurationController,
                                                keyboardType:
                                                    TextInputType.number,
                                                decoration:
                                                    const InputDecoration(
                                                  labelText: '执行时间',
                                                  hintText: '毫秒，例如 500',
                                                ),
                                              ),
                                            ),
                                          ],
                                        ),
                                        const SizedBox(height: 16),
                                        Align(
                                          alignment: Alignment.centerLeft,
                                          child: FilledButton(
                                            onPressed: _sendingPress
                                                ? null
                                                : _sendPressCommand,
                                            child: Text(
                                              _sendingPress ? '发送中...' : '按一下再松开',
                                            ),
                                          ),
                                        ),
                                      ],
                                    ),
                                  ),
                                ),
                                const SizedBox(height: 20),
                                _SectionCard(
                                  child: _ControlSection(
                                    title: '任务模式',
                                    description: '按指定角度往返执行任务，执行时间越长，移动速度越慢。',
                                    child: Column(
                                      children: [
                                        Row(
                                          children: [
                                            Expanded(
                                              child: TextField(
                                                controller: _taskAngleController,
                                                keyboardType:
                                                    TextInputType.number,
                                                decoration:
                                                    const InputDecoration(
                                                  labelText: '任务角度',
                                                  hintText: '0-180',
                                                ),
                                              ),
                                            ),
                                            const SizedBox(width: 12),
                                            Expanded(
                                              child: TextField(
                                                controller: _taskCountController,
                                                keyboardType:
                                                    TextInputType.number,
                                                decoration:
                                                    const InputDecoration(
                                                  labelText: '往返次数',
                                                  hintText: '1',
                                                ),
                                              ),
                                            ),
                                          ],
                                        ),
                                        const SizedBox(height: 12),
                                        TextField(
                                          controller: _taskDurationController,
                                          keyboardType: TextInputType.number,
                                          decoration: const InputDecoration(
                                            labelText: '单程执行时间',
                                            hintText: '毫秒，例如 800',
                                          ),
                                        ),
                                        const SizedBox(height: 16),
                                        Align(
                                          alignment: Alignment.centerLeft,
                                          child: FilledButton.tonal(
                                            onPressed:
                                                _sendingTask ? null : _sendTaskCommand,
                                            child: Text(
                                              _sendingTask
                                                  ? '发送中...'
                                                  : '执行往返任务',
                                            ),
                                          ),
                                        ),
                                      ],
                                    ),
                                  ),
                                ),
                                const SizedBox(height: 20),
                                _SectionCard(
                                  child: Column(
                                    crossAxisAlignment: CrossAxisAlignment.start,
                                    children: [
                                      Text(
                                        '反馈信息',
                                        style: theme.textTheme.titleMedium?.copyWith(
                                          fontWeight: FontWeight.w700,
                                        ),
                                      ),
                                      const SizedBox(height: 12),
                                      Text(_feedback),
                                    ],
                                  ),
                                ),
                              ],
                            ),
                          ),
                          SizedBox(width: wide ? 20 : 0, height: wide ? 0 : 20),
                          Expanded(
                            flex: 2,
                            child: _SectionCard(
                              accent: const Color(0xFF12372A),
                              child: _StatusPane(
                                status: _status,
                                onRefresh: _fetchStatus,
                              ),
                            ),
                          ),
                        ],
                      ),
                    ),
                  ],
                ),
              ),
            ),
          ),
        ),
      ),
    );
  }
}

class _AngleBadge extends StatelessWidget {
  const _AngleBadge({required this.angle});

  final int angle;

  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);

    return Container(
      width: 180,
      padding: const EdgeInsets.symmetric(horizontal: 20, vertical: 18),
      decoration: BoxDecoration(
        color: const Color(0xFF12372A),
        borderRadius: BorderRadius.circular(24),
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Text(
            '当前角度',
            style: theme.textTheme.titleSmall?.copyWith(
              color: Colors.white.withValues(alpha: 0.9),
            ),
          ),
          const SizedBox(height: 8),
          Text(
            '$angle°',
            style: theme.textTheme.displaySmall?.copyWith(
              color: Colors.white,
              fontWeight: FontWeight.w800,
            ),
          ),
        ],
      ),
    );
  }
}

class _ControlSection extends StatelessWidget {
  const _ControlSection({
    required this.title,
    required this.description,
    required this.child,
  });

  final String title;
  final String description;
  final Widget child;

  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);

    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Text(
          title,
          style: theme.textTheme.titleLarge?.copyWith(
            fontWeight: FontWeight.w800,
            color: const Color(0xFF12372A),
          ),
        ),
        const SizedBox(height: 8),
        Text(
          description,
          style: theme.textTheme.bodyMedium?.copyWith(
            color: const Color(0xFF4C6A61),
          ),
        ),
        const SizedBox(height: 20),
        child,
      ],
    );
  }
}

class _StatusPane extends StatelessWidget {
  const _StatusPane({
    required this.status,
    required this.onRefresh,
  });

  final DeviceStatus? status;
  final Future<void> Function({bool silent}) onRefresh;

  String _mapStatus(String value) {
    switch (value) {
      case 'ready':
        return '就绪';
      case 'running':
        return '任务执行中';
      case 'positioning':
        return '定位中';
      case 'positioned':
        return '已定位';
      case 'completed':
        return '任务完成';
      default:
        return value;
    }
  }

  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);

    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Row(
          children: [
            Expanded(
              child: Text(
                '设备状态',
                style: theme.textTheme.titleLarge?.copyWith(
                  fontWeight: FontWeight.w700,
                ),
              ),
            ),
            OutlinedButton(
              onPressed: () => onRefresh(),
              child: const Text('刷新状态'),
            ),
          ],
        ),
        const SizedBox(height: 20),
        if (status == null)
          const Text('尚未获取设备状态。')
        else
          ...<MapEntry<String, String>>[
            MapEntry('设备 ID', status!.deviceId),
            MapEntry('当前角度', '${status!.currentAngle}°'),
            MapEntry('设备忙碌', status!.busy ? '是' : '否'),
            MapEntry('状态文本', _mapStatus(status!.statusMessage)),
            MapEntry('最近请求', '${status!.lastCompletedRequestId}'),
            MapEntry('Wi‑Fi', status!.wifiConnected ? '已连接' : '未连接'),
            MapEntry('设备 IP', status!.ip),
            MapEntry('Wi‑Fi SSID', status!.ssid),
          ].map(
            (row) => Padding(
              padding: const EdgeInsets.only(bottom: 12),
              child: Row(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  SizedBox(
                    width: 96,
                    child: Text(
                      row.key,
                      style: theme.textTheme.bodyMedium?.copyWith(
                        fontWeight: FontWeight.w700,
                      ),
                    ),
                  ),
                  Expanded(child: Text(row.value)),
                ],
              ),
            ),
          ),
      ],
    );
  }
}

class _SectionCard extends StatelessWidget {
  const _SectionCard({
    required this.child,
    this.accent = const Color(0xFF0B6E4F),
  });

  final Widget child;
  final Color accent;

  @override
  Widget build(BuildContext context) {
    return Container(
      width: double.infinity,
      padding: const EdgeInsets.all(24),
      decoration: BoxDecoration(
        color: Colors.white.withValues(alpha: 0.88),
        borderRadius: BorderRadius.circular(28),
        border: Border.all(color: accent.withValues(alpha: 0.12)),
        boxShadow: [
          BoxShadow(
            color: accent.withValues(alpha: 0.08),
            blurRadius: 32,
            offset: const Offset(0, 20),
          ),
        ],
      ),
      child: child,
    );
  }
}

class DeviceStatus {
  DeviceStatus({
    required this.deviceId,
    required this.busy,
    required this.currentAngle,
    required this.lastCompletedRequestId,
    required this.statusMessage,
    required this.wifiConnected,
    required this.ip,
    required this.ssid,
  });

  final String deviceId;
  final bool busy;
  final int currentAngle;
  final int lastCompletedRequestId;
  final String statusMessage;
  final bool wifiConnected;
  final String ip;
  final String ssid;

  factory DeviceStatus.fromJson(Map<String, dynamic> json) {
    return DeviceStatus(
      deviceId: json['deviceId']?.toString() ?? '未知',
      busy: json['busy'] == true,
      currentAngle: (json['currentAngle'] as num?)?.toInt() ?? 0,
      lastCompletedRequestId:
          (json['lastCompletedRequestId'] as num?)?.toInt() ?? 0,
      statusMessage: json['statusMessage']?.toString() ?? '未知',
      wifiConnected: json['wifiConnected'] == true,
      ip: json['ip']?.toString() ?? '',
      ssid: json['ssid']?.toString() ?? '',
    );
  }
}
