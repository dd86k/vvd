@ECHO OFF

CD ..

FOR /F %%G IN (utils\gpt_types) DO (
	vvd internalguidhash %%G >> utils\gpt_hashes
	IF ERRORLEVEL 1 GOTO :EOF
)

CD utils