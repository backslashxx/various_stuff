const std = @import("std");
const linux = std.os.linux;

// zig build-exe hello.zig -O ReleaseSmall --gc-sections -fstrip

fn add_byref_to_char(a: *const i32, b: *const i32, buf: *[2]u8) void {
	const x: i32 = a.* + b.*;
	buf[0] = 48 + @as(u8, @intCast(x));
	buf[1] = '\n';
}

pub fn main() void {
	var a: i32 = 1;
	var b: i32 = 2;
	var buf: [2]u8 = undefined;

	add_byref_to_char(&a, &b, &buf);

	_ = linux.write(1, &buf, 2);
	linux.exit(0);

}
