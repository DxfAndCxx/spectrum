# spectrum


## arch
spectrum works with lua. So you can filter, control, print by lua.

The all work is split to `records read`, `records iter`, `summary`.
The first stage works with multi threads. So you should know that the lua
function is run at a thread, every thread has it's own lua state.

## sp
This global var is the top value of spectrum;

### sp.opt
This is used to set the options. When everything is ok, event `spectrum_config`
is called, so you can set the options in the function `spectrum_config`.

like this:
```
function spectrum_config()
    sp.opt.file_log = 't/ngx_logs'
    sp.opt.file_pattern = 't/pattern'
end
```

### sp.record
`record` is the field of `sp`, it is pointed to the current record when the
records ware readed. When every record is readed,  the event
`spectrum_record_read` is called. You can read the fields by `sp.record.vars`,
there fields is readonly. You just can append new fields, cannot the fields.
Appends new field by `sp.record.append`.

If you this the record is not needed, then you can drop the record by
`sp.record.drop`.

### sp.pattern
You can access the pattern by this.

## command line args
### -s
start server with out socket listen. This mode just run records read.
### -S
After read records, when start a socket server to recv the cmd from client.
### -c [cmd]
Run as client, send the cmd to server.
### -f [rc.lua]
The default rc.lua is spectrum.lua in the current pwd. You can set the rc.lua by
-f.

#### cmd
* stop: stop the server
* iter: server will fork a process, and start threads to iter all records.
event `spectrum`

