@echo off

lang alias.mt
mint out.mb > alias.log 2> alias.err

lang coroutine.mt coroutines.mt
mint out.mb > coroutine.log 2> coroutine.err

lang lambda.mt
mint out.mb > lambda.log 2> lambda.err

lang macros.mt
mint out.mb > macros.log 2> macros.err

lang operator.mt
mint out.mb > operator.log 2> operator.err

lang sort.mt
mint out.mb < sort_input.txt > sort.log 2> sort.err

lang typeinfo.mt
mint out.mb > typeinfo.log 2> typeinfo.err

del out.mb
