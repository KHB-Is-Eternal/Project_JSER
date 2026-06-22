#include "CharacterSystem/GameplayTags/GameplayTags.h"

namespace ProjectER
{
	namespace Ability
	{
		namespace Action
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(AutoAttack, "Ability.Action.AutoAttack", "Attack Ability");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Death, "Ability.Action.Death", "Death Ability");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Revive, "Ability.Action.Revive", "Revive Ability");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Move, "Ability.Action.Move", "Move Ability");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Chase, "Ability.Action.Chase", "Chase Ability");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Return, "Ability.Action.Return", "Return Ability");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Alert, "Ability.Action.Alert", "Alert Ability");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Idle, "Ability.Action.Idle", "Idle Ability");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Combat, "Ability.Action.Combat", "Combat Ability");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Interaction, "Ability.Action.Interaction", "Interaction Ability");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(FlyStart, "Ability.Action.FlyStart", "FlyStart Ability");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(FlyAttack, "Ability.Action.FlyAttack", "FlyAttack Ability");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(FlyEnd, "Ability.Action.FlyEnd", "FlyEnd Ability");
		}

		namespace Input
		{
			namespace Item
			{
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Slot_1, "Ability.Input.Item.Slot_1", "Item Slot 1 Input");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Slot_2, "Ability.Input.Item.Slot_2", "Item Slot 2 Input");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Slot_3, "Ability.Input.Item.Slot_3", "Item Slot 3 Input");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Slot_4, "Ability.Input.Item.Slot_4", "Item Slot 4 Input");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Slot_5, "Ability.Input.Item.Slot_5", "Item Slot 5 Input");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Slot_6, "Ability.Input.Item.Slot_6", "Item Slot 6 Input");

			}

			namespace Skill
			{
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Passive, "Ability.Input.Skill.Passive", "Passive Skill Input");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Q, "Ability.Input.Skill.Q", "Q Skill Input");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(W, "Ability.Input.Skill.W", "W Skill Input");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(E, "Ability.Input.Skill.E", "E Skill Input");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(R, "Ability.Input.Skill.R", "Ultimate Skill Input");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(D, "Ability.Input.Skill.D", "Weapon Skill Input");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(F, "Ability.Input.Skill.F", "");
			}
		}
	}

	namespace Cooldown 
	{
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(AutoAttack, "Cooldown.AutoAttack", "Auto Attack Cooldown");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Global, "Cooldown.Global", "Global Cooldown");

		namespace Item
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Use, "Cooldown.Item.Use", "Using Item Cooldown");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Set, "Cooldown.Item.Set", "Setting Item Cooldown");

		}

		namespace Skill
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Passive, "Cooldown.Skill.Passive", "Passive Skill Cooldown");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Q, "Cooldown.Skill.Q", "Q Skill Cooldown");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(W, "Cooldown.Skill.W", "W Skill Cooldown");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(E, "Cooldown.Skill.E", "E Skill Cooldown");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(R, "Cooldown.Skill.R", "Ultimate Skill Cooldown");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(D, "Cooldown.Skill.D", "Weapon Skill Cooldown");
		}
	}

	namespace Data
	{
		namespace DamageType
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Physical, "Data.DamageType.Physical", "Physical Damage");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Skill, "Data.DamageType.Skill", "Skill (Magic) Damage");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(True, "Data.DamageType.True", "True Damage (Ignores Armor)");
		}

		namespace Amount
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Health, "Data.Amount.Heal", "Heal Amount");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Damage, "Data.Amount.Damage", "Damage Amount");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(IncomingXP, "Data.Amount.IncomingXP", "Incoming XP Amount");
		}
		
		namespace CC
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Duration, "Data.CC.Duration", "CC Duration (SetByCaller)");
		}

		namespace Source
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Skill, "Data.Source.Skill", "From Skill");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attack, "Data.Source.Attack", "From Attack");
		}
	}


	
	namespace Event
	{
		namespace Data
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Damage, "Event.Data.Damage", "Event data for local damage amount");
		}

		namespace Action
		{
			namespace Hit
			{
				namespace BasicAttack
				{
					UE_DEFINE_GAMEPLAY_TAG_COMMENT(Critical, "Event.Action.Hit.BasicAttack.Critical", "Event for Hit BasicAttack Critical");
				}
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Damaged, "Event.Action.Hit.Damaged", "Event for Hit Damaged");
				
				namespace Skill
				{
					UE_DEFINE_GAMEPLAY_TAG_COMMENT(Q, "Event.Action.Hit.Skill.Q", "Event for Hit Skill");
					UE_DEFINE_GAMEPLAY_TAG_COMMENT(W, "Event.Action.Hit.Skill.W", "Event for Hit Skill");
					UE_DEFINE_GAMEPLAY_TAG_COMMENT(E, "Event.Action.Hit.Skill.E", "Event for Hit Skill");
					UE_DEFINE_GAMEPLAY_TAG_COMMENT(R, "Event.Action.Hit.Skill.R", "Event for Hit Skill");
					UE_DEFINE_GAMEPLAY_TAG_COMMENT(Passive, "Event.Action.Hit.Skill.Passive", "Event for Hit Skill");
				}
			}
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attack, "Event.Action.Attack", "Event for Attack");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(BeginSearch, "Event.Action.BeginSearch", "Monster BeginSearch");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(EndSearch, "Event.Action.EndSearch", "Monster EndSearch");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(TargetOn, "Event.Action.TargetOn", "Event for TargetOn");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(TargetOff, "Event.Action.TargetOff", "Event for TargetOff");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Interaction, "Event.Action.Interaction", "Event for Interaction");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Death, "Event.Action.Death", "Event for Death");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Return, "Event.Action.Return", "Event for Return");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Phase1, "Event.Action.Phase1", "Event for Phase1");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Phase2, "Event.Action.Phase2", "Event for Phase2");

			namespace Skill
			{
				namespace Execute
				{
					UE_DEFINE_GAMEPLAY_TAG_COMMENT(Q, "Event.Action.Skill.Execute.Q", "When Skill Execute Q");
					UE_DEFINE_GAMEPLAY_TAG_COMMENT(W, "Event.Action.Skill.Execute.W", "When Skill Execute W");
					UE_DEFINE_GAMEPLAY_TAG_COMMENT(E, "Event.Action.Skill.Execute.E", "When Skill Execute E");
					UE_DEFINE_GAMEPLAY_TAG_COMMENT(R, "Event.Action.Skill.Execute.R", "When Skill Execute R");
					UE_DEFINE_GAMEPLAY_TAG_COMMENT(Passive, "Event.Action.Skill.Execute.Passive", "When Skill Execute Passive");
				}

				namespace End
				{
					UE_DEFINE_GAMEPLAY_TAG_COMMENT(Q, "Event.Action.Skill.End.Q", "When Skill End Q");
					UE_DEFINE_GAMEPLAY_TAG_COMMENT(W, "Event.Action.Skill.End.W", "When Skill End W");
					UE_DEFINE_GAMEPLAY_TAG_COMMENT(E, "Event.Action.Skill.End.E", "When Skill End E");
					UE_DEFINE_GAMEPLAY_TAG_COMMENT(R, "Event.Action.Skill.End.R", "When Skill End R");
					UE_DEFINE_GAMEPLAY_TAG_COMMENT(Passive, "Event.Action.Skill.End.Passive", "When Skill End Passive");
				}
			}
		}

		namespace State
		{
			namespace Debuff
			{
				namespace Soft // 행동 제약
				{
					UE_DEFINE_GAMEPLAY_TAG_COMMENT(Slow, "Event.State.Debuff.Soft.Slow", "Event for Slow");    // 둔화
					UE_DEFINE_GAMEPLAY_TAG_COMMENT(Root, "Event.State.Debuff.Soft.Root", "Event for Root");    // 속박
					UE_DEFINE_GAMEPLAY_TAG_COMMENT(Silence, "Event.State.Debuff.Soft.Silence", "Event for Silence"); // 침묵
				}
				namespace Hard // 행동 불가
				{
					UE_DEFINE_GAMEPLAY_TAG_COMMENT(Stun, "Event.State.Debuff.Hard.Stun", "Event for Stun");     // 기절
					UE_DEFINE_GAMEPLAY_TAG_COMMENT(Airborne, "Event.State.Debuff.Hard.Airborne", "Event for Airborne"); // 에어본
				}

				UE_DEFINE_GAMEPLAY_TAG_COMMENT(BlockRegen, "Event.State.Debuff.BlockRegen", "Event for BlockRegen");    // 회복 불가
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(ReduceHealing, "Event.State.Debuff.ReduceHealing", "Event for ReduceHealing"); // 치유 감소 (치감)
			}
		}

		namespace Montage
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(AttackHit, "Event.Montage.AttackHit", "Montage Event for Attack Hit");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(SpawnProjectile, "Event.Montage.SpawnProjectile", "Montage Event for Spawn Projectile");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Active, "Event.Montage.Active", "Montage Event for Active");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Casting, "Event.Montage.Casting", "Montage Event for Casting");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(End, "Event.Montage.End", "Montage Event for End");
		}

		namespace System
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Test, "Event.System.Test", "");
		}

		namespace UI
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Test, "Event.UI.Test", "");
		}

		namespace Interact
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(OpenBox, "Event.Interact.OpenBox", "");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Teleport, "Event.Interact.Teleport", "Teleport Interaction");
		}


	}

	namespace GameplayCue
	{
		namespace Particle
		{
			namespace Action
			{
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Death, "GameplayCue.Particle.Action.Death", "");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Alert, "GameplayCue.Particle.Action.Alert", "");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Idle, "GameplayCue.Particle.Action.Idle", "");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Combat, "GameplayCue.Particle.Action.Combat", "");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Move, "GameplayCue.Particle.Action.Move", "");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(FlyStart, "GameplayCue.Particle.Action.FlyStart", "");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(FlyAttack, "GameplayCue.Particle.Action.FlyAttack", "");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(FlyEnd, "GameplayCue.Particle.Action.FlyEnd", "");
			}
			namespace Skill
			{
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(AutoAttack, "GameplayCue.Particle.Skill.AutoAttack", "AutoAttack Particle");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Q, "GameplayCue.Particle.Skill.Q", "Q Particle");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(W, "GameplayCue.Particle.Skill.W", "W Particle");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(E, "GameplayCue.Particle.Skill.E", "E Particle");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(R, "GameplayCue.Particle.Skill.R", "R Particle");
			}
		}
		namespace Sound
		{
			namespace Action
			{
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Death, "GameplayCue.Sound.Action.Death", "");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Alert, "GameplayCue.Sound.Action.Alert", "");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Idle, "GameplayCue.Sound.Action.Idle", "");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Combat, "GameplayCue.Sound.Action.Combat", "");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Move, "GameplayCue.Sound.Action.Move", "");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(FlyStart, "GameplayCue.Sound.Action.FlyStart", "");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(FlyAttack, "GameplayCue.Sound.Action.FlyAttack", "");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(FlyEnd, "GameplayCue.Sound.Action.FlyEnd", "");
			}
			namespace Skill
			{
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(AutoAttack, "GameplayCue.Sound.Skill.AutoAttack", "AutoAttack Particle");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Q, "GameplayCue.Sound.Skill.Q", "Q Particle");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(W, "GameplayCue.Sound.Skill.W", "W Particle");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(E, "GameplayCue.Sound.Skill.E", "E Particle");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(R, "GameplayCue.Sound.Skill.R", "R Particle");
			}
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Summoner, "GameplayCue.Sound.Summoner", "");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(HitTarget, "GameplayCue.Sound.HitTarget", "");
		}
		namespace Decal
		{
			namespace Skill
			{
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(AutoAttack, "GameplayCue.Decal.Skill.AutoAttack", "AutoAttack Decal");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Q, "GameplayCue.Decal.Skill.Q", "Q Decal");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(W, "GameplayCue.Decal.Skill.W", "W Decal");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(E, "GameplayCue.Decal.Skill.E", "E Decal");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(R, "GameplayCue.Decal.Skill.R", "R Decal");
			}
		}

		namespace Combat
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Hit, "GameplayCue.Combat.Hit", "Cue for Attack Hit");
			namespace HitEffect
			{
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Aurora, "GameplayCue.Combat.HitEffect.Aurora", "Cue for Aurora Auto Attack Hit");
			}
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Revive, "GameplayCue.Combat.Revive", "Cue for Revive");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(TryRevive, "GameplayCue.Combat.TryRevive", "Cue for Try Revive Ally");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(LevelUp, "GameplayCue.Combat.LevelUp", "Cue for Level Up");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Death, "GameplayCue.Combat.Death", "Cue for Death");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(DamageText, "GameplayCue.Combat.DamageText", "Cue for Spawn Damage Floating Text");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(RecoveryText, "GameplayCue.Combat.RecoveryText", "Cue for Spawn Recovery Floating Text");
		}

		namespace Skill
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Actor, "GameplayCue.Skill.Actor", "");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Summoner, "GameplayCue.Skill.Summoner", "Cue for Skill SummonVFX");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(HitTarget, "GameplayCue.Skill.HitTarget", "Cue for HitTargetVFX");
		}
		
		namespace CC
        {
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Slow, "GameplayCue.CC.Slow", "Slow VFX/SFX");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Root, "GameplayCue.CC.Root", "Root VFX/SFX");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Silence, "GameplayCue.CC.Silence", "Silence VFX/SFX");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Stun, "GameplayCue.CC.Stun", "Stun VFX/SFX");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Airborne, "GameplayCue.CC.Airborne", "Airborne VFX/SFX");
        }
            
	}
	
	namespace State
	{
		namespace Action
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Interaction, "State.Action.Interaction", "Interaction State");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Casting, "State.Action.Casting", "Casting State");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Combat, "State.Action.Combat", "Combat State");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Alert, "State.Action.Alert", "Alert State");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Idle, "State.Action.Idle", "Idle State");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Move, "State.Action.Move", "Move State");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attack, "State.Action.Attack", "Attack State");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Reviving, "State.Action.Reviving", "Reviving State");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(FlyStart, "State.Action.FlyStart", "FlyStart State");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(FlyAttack, "State.Action.FlyAttack", "FlyAttack State");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(FlyEnd, "State.Action.FlyEnd", "FlyEnd State");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Skill, "State.Action.Skill", "Skill State");
		}

		namespace Buff
		{
			namespace Immune
			{
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(CC, "State.Buff.Immune.CC", "Immune to CC effects State");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Damage, "State.Buff.Immune.Damage", "Immune to Damage (Invulnerable) State");
			}
		}

		namespace Debuff
		{
			namespace Soft
			{
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Slow, "State.Debuff.Soft.Slow", "Movement Speed Slow");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Root, "State.Debuff.Soft.Root", "Cannot Move");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Silence, "State.Debuff.Soft.Silence", "Cannot use Skills");
			}
			namespace Hard
			{
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Stun, "State.Debuff.Hard.Stun", "Cannot Move or Act");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Airborne, "State.Debuff.Hard.Airborne", "Knocked Up (Airborne)");
			}

			UE_DEFINE_GAMEPLAY_TAG_COMMENT(BlockRegen, "State.Debuff.BlockRegen", "Cannot Regenerate Health");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(ReduceHealing, "State.Debuff.ReduceHealing", "Healing effectiveness reduced");
		}

		namespace Life
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Death, "State.Life.Death", "Death State");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Down, "State.Life.Down", "Groggy State");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Alive, "State.Life.Alive", "Alive State");
		}
		
		/*namespace Status
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Unselectable, "State.Status.Unselectable", "Unselectable State"); // 적 타겟팅은 되지만 일부 스킬 불가(Down 시 부여)
		}*/

		namespace Zone
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Safe, "State.Zone.Safe", "Place SafeZone"); // 임시 안전 지대에 위치
		}
	}

	namespace Status
	{
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Level, "Status.Level", "Current Level");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(MaxLevel, "Status.MaxLevel", "Max Level");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(XP, "Status.XP", "Current XP");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(MaxXP, "Status.MaxXP", "XP required for next level");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Health, "Status.Health", "Current Health");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(MaxHealth, "Status.MaxHealth", "Maximum Health");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(HealthRegen, "Status.HealthRegen", "Health Regeneration per second");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Stamina, "Status.Stamina", "Current Stamina/Mana");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(MaxStamina, "Status.MaxStamina", "Maximum Stamina/Mana");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(StaminaRegen, "Status.StaminaRegen", "Stamina Regeneration per second");
		
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(AttackPower, "Status.AttackPower", "Physical Attack Power");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(AttackSpeed, "Status.AttackSpeed", "Attack Speed"); 
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(AttackRange, "Status.AttackRange", "Attack Range"); 
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(SkillAmp, "Status.SkillAmp", "Skill Amplification");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(CritChance, "Status.CritChance", "Critical Hit Chance");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(CritDamage, "Status.CritDamage", "Critical Hit Damage Multiplier");
		
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Defense, "Status.Defense", "Physical/Skill Defense");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(MoveSpeed, "Status.MoveSpeed", "Movement Speed");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(CooldownReduction, "Status.CooldownReduction", "Cooldown Reduction %");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Tenacity, "Status.Tenacity", "Crowd Control Reduction %");
	}
	
	namespace Team
	{
		namespace Relation
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Self, "Team.Relation.Self", "Target is Self");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Friendly, "Team.Relation.Friendly", "Target is Friendly");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Hostile, "Team.Relation.Hostile", "Target is Hostile");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Neutral, "Team.Relation.Neutral", "Target is Neutral");
		}
	}
	
	namespace Unit 
	{
		namespace AttackType
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Melee, "Unit.AttackType.Melee", "Melee Attack Range");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ranged, "Unit.AttackType.Ranged", "Ranged Attack Range");
		}

		namespace Type 
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Player, "Unit.Type.Player", "Unit is a Player Character");
			
			namespace Monster 
			{
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Mob, "Unit.Type.Monster.Mob", "Normal Monster");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Epic, "Unit.Type.Monster.Epic", "Epic Monster");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Boss, "Unit.Type.Monster.Boss", "Boss Monster");
			}
			
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Structure, "Unit.Type.Structure", "Static Structures");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Object, "Unit.Type.Object", "Interactable Objects");
		}
	}

	namespace Montage
	{
		namespace Common
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Death, "Montage.Common.Death", "Death Action Montage");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(AutoAttack, "Montage.Common.AutoAttack", "AutoAttack Action Montage");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(PrimaryA, "Montage.Common.PrimaryA", "Primary A Action Montage");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(PrimaryB, "Montage.Common.PrimaryB", "Primary B Action Montage");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(PrimaryC, "Montage.Common.PrimaryB", "Primary C Action Montage");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(CriticalHit, "Montage.Common.CriticalHit", "CriticalHit Action Montage");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(TryRevive, "Montage.Common.TryRevive", "TryRevive Action Montage");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Revive, "Montage.Common.Revive", "Revive Action Montage");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Stun, "Montage.Common.Stun", "Stun Reaction Montage");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Airborne, "Montage.Common.Airborne", "Airborne Reaction Montage");
		}
	}

	namespace Skill
	{
		namespace Animation
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Casting, "Skill.Animation.Casting", "Skill Casting State Tag");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Active, "Skill.Animation.Active", "Skill Active (Hit) State Tag");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Backswing, "Skill.Animation.Backswing", "Skill Backswing State Tag");
		}
		namespace Data
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(CoolTime, "Skill.Data.CoolTime", "Cooldown Tag for SetByCaller");
		}
	}
}