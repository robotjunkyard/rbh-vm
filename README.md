# rbh-vm  ( Robots For Hire VM )
*A virtual RISC(-ish) machine library in C++*

This is intended to be a component used in what will a larger long-term project; an artificial digital-life game/simulator using
colony-driven behavior, mainly for a programmer audience or people who want an excuse to learn assembly language.  RBH-VM is of
course specifically the Virtual Machine, intended to be compact and tiny with about four general-purpose registers and 8K ROM,
and about 44 Opcodes (a couple probably need to be tested).

Currently RBH-VM is not a "library" yet (not until it's stable, as I'm *certain* there are some creeping bugs waiting to show up), as it is still under development.  As of now, uses Omar Cornut's excellent ImGui (its official page is at https://github.com/ocornut/imgui) for the "dashboard" that allows you to easily view the source and step through the register history of the machine's operation.

Copyright 2016-2017 Nick Baker  <email: njb at robotjunkyard dot org>

