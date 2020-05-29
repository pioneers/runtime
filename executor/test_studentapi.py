import studentapi

# Add tests for API

robot = studentapi.Robot()
val = robot.get_value('23840928345902384752093485', 'duty')
print(val)