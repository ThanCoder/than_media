// ignore_for_file: non_constant_identifier_names

part of 'audio_device.dart';

mixin AudioDeviceStateHandler {
  // Stream Controllers
  final _stateController = StreamController<PlayerState>.broadcast();
  final _positionController = StreamController<double>.broadcast();
  final _durationController = StreamController<double>.broadcast();

  // Public Streams for UI
  Stream<PlayerState> get stateStream => _stateController.stream;
  Stream<double> get positionStream => _positionController.stream;
  Stream<double> get durationStream => _durationController.stream;

  PlayerState _currentState = PlayerState.stopped;
  PlayerState get currentState => _currentState;

  Timer? _positionTimer;
  Pointer<Void> get _audio_device_ptr;

  // Timer မောင်းပြီး Position ကို ခေါ်ယူခြင်း
  void _startPositionTimer() {
    _positionTimer?.cancel();
    // 200 milliseconds တိုင်း တစ်ကြိမ် C++ ဘက်က လက်ရှိစက္ကန့်ကို လှမ်းတောင်းမယ်
    _positionTimer = Timer.periodic(const Duration(milliseconds: 200), (timer) {
      if (_audio_device_ptr != nullptr) {
        final current = audio_device_getCurrentInSeconds(_audio_device_ptr);
        final total = audio_device_getDurationInSeconds(_audio_device_ptr);

        _positionController.add(current);

        // သီချင်းပြီးသွားပြီလား စစ်ဆေးခြင်း (C++ ဘက်က complete callback မပါရင် ဒီလိုစစ်ရပါတယ်)
        if (current >= total && total > 0) {
          _updateState(PlayerState.completed);
          _stopPositionTimer();
        }
      }
    });
  }

  void _stopPositionTimer() {
    _positionTimer?.cancel();
  }

  void _updateState(PlayerState state) {
    _currentState = state;
    _stateController.add(state);
  }
}
